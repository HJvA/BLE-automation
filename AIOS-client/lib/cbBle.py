""" interface to pythonista version cb of 'CoreBluetooth'
		discovers specific devices (bluetooth peripherals) and their characteristics
		reads/writes from/to characteristics
"""
import cb
import time
from functools import partial
import logging
if '.' in __name__:  # imported from higher level package
	from . import tls
	logger = tls.get_logger()
else:
	import tls   # gives error when called from main package also using tls
	logger = tls.get_logger(__file__, logging.DEBUG)

cbPropMsg = [
	(cb.CH_PROP_AUTHENTICATED_SIGNED_WRITES, 'Authenticated'),
	(cb.CH_PROP_BROADCAST, 'Broadcast'),
	(cb.CH_PROP_EXTENDED_PROPERTIES, 'Extended'),
	(cb.CH_PROP_INDICATE, 'Indicate'),
	(cb.CH_PROP_INDICATE_ENCRYPTION_REQUIRED, 'Encryption Required'),
	(cb.CH_PROP_NOTIFY, 'Notify'),
	(cb.CH_PROP_NOTIFY_ENCRYPTION_REQUIRED, 'Notif.Encr.Req'),
	(cb.CH_PROP_READ, 'Read'),
	(cb.CH_PROP_WRITE, 'Write'),
	(cb.CH_PROP_WRITE_WITHOUT_RESPONSE, 'Write no response')
	]

# dict characteristic keys for perepheral scanning in lod:lookup dict
chPERF = 0  # peripheral name
chID   = 1  # internal id
chPUID = 2  # periphral uuid
chSUID = 3  # service uuid
chCUID = 4  # characteristic uuid
chCallback = 5

def MaskedPropMsg(msgDict, bitmask):
	lst = (k[1] for k in msgDict if k[0] & bitmask)
	return ','.join(lst)

class bleDelegate (object):
	""" having prescibed interface for cb module
	"""
	def __init__(self, lodBLE):
		"""
		args
		lodBLE : list of dicts describing peripherals and characteristics to be found
			[{ chPERF:part of PeripheralName, chID:just an identifying number,
				chPUID:part of peripheral uuid, chCUID:part of characteristic uuid},...]
		"""
		self.peripherals = []
		self.lodCharist = lodBLE
		self.ToBeFnd = set([r[chID] for r in self.lodCharist])
		logger.info('### ble Scanning %d peripherals ###' % len(self.ToBeFnd))
		self.charFound = []
		self.updating = 0
		# self.respCallback=self._exampleRespCallback

	def close(self):
		""" terminate ble activity
		"""
		cb.reset()
		
	def allFound(self):
		return len(self.ToBeFnd) == 0
		
	def _inLod(self, perf, charis=None):
		""" get lodrecs of charis to be found for peripheral
		"""
		if not perf.name:
			return []
		isin = tls.query_lod(self.lodCharist,
		lambda rw: rw[chPERF] in perf.name and (not rw[chPUID]) and rw[chCUID])
		if not isin:
			isin = tls.query_lod(self.lodCharist, 
			lambda rw: (rw[chPUID] and rw[chPUID] in perf.uuid))
		if not isin:
			isin = tls.query_lod(self.lodCharist,
			lambda rw: rw[chPERF] in perf.name and (not rw[chPUID]) and (not rw[chCUID]))
		if charis:
			isin = tls.query_lod(isin,
			lambda rw: ((rw[chCUID] in charis.uuid) and (rw[chID] in self.ToBeFnd)))
		return isin

	def _PerfQueueSyncer(self, perf, timeout=5):
		''' check whether perf has some characteristics to be discovered  '''
		isin = self._inLod(perf)
		if not isin:
			logger.error('nothing assigned for perf:%s' % perf.name)
			return False
		else:
			return True
									
	def did_discover_peripheral(self, p):
		if not p.name:
			return
		isin = self._inLod(p)
		logger.info('testing peripheral: %s state %d (%s) isin:%d' % (p.name,p.state, p.uuid, len(isin)))
		if p.state > 0:   # allready connecting
			return
		if len(isin) > 0:   # matching lod
			# self.charFound.append(dict(perf=p))
			self.peripherals.append(p) # keep reference!
			logger.info('connecting %s' % p.name)
			cb.connect_peripheral(p)
			
	def did_connect_peripheral(self, p):
		logger.info('*** Connected: %s (%s)' % (p.name, p.uuid))
		p.discover_services()

	def did_fail_to_connect_peripheral(self, p, error):
		logger.error('Failed to connect %s because %s' % (p.name, error))
		# self.peripherals.remove(p)

	def did_disconnect_peripheral(self, p, error):
		logger.info('Disconnected %s, error: %s' % (p.name, error))
		self.peripherals.remove(p)

	def did_discover_services(self, p, error):
		logger.info('found %d services for %s (%s) busy:%d' % (len(p.services), p.name, p.uuid, len(self.ToBeFnd)))
		if self._PerfQueueSyncer(p):
			logger.info('discovering characteristics n:%d for %s' % (len(self.ToBeFnd),p.name))
			for s in p.services:
				if s.primary:
					p.discover_characteristics(s)
				else:
					logger.info('%s primary:%d' % (s.uuid, s.primary))
			
	def did_discover_characteristics(self, serv, error):
		logger.info('%d characteristics for serv:%s (prim:%s)' % (len(serv.characteristics), serv.uuid, serv.primary))
		for c in serv.characteristics:
			if len(self.ToBeFnd) == 0:
				logger.info('nothing to discover anymore for act peripheral')
				break
			else:
				perf = None
				for p in self.peripherals:	# find actual peripheral with service
					if p.services: 
						for srv in p.services:
							if serv.uuid == srv.uuid:
								logger.debug('fnd:%s with s:%s c:%s' % (p.name, serv.uuid, c.uuid))
								perf = p
								break
				if not perf:
					logger.error('perf %s not found in %s' & (p.uuid, serv.uuid))
					continue
				isin = self._inLod(perf, c)
				if isin:
					self.ToBeFnd.discard(isin[0][chID])
					if hasattr(c,'descriptors'):
						logger.info('descriptors:%s' % c.descriptors)
					logger.info("++ charist %d %s (notif:%d)-->%s " %
							(isin[0][chID], c.uuid, c.notifying, MaskedPropMsg(cbPropMsg, c.properties)))
					# debug.stop()
					self.charFound.append({chID:isin[0][chID], 'periph':perf, 'charis':c, chCallback:None})
				else:
					logger.info('not %s isin %s p:%s ' % (c.uuid, self.ToBeFnd, perf.name))
					
	def did_write_value(self, c, error):
		logger.debug('Did write val to c:%s val:%s' % (c.uuid,''.join('{:02x}'.format(x) for x in c.value)))

	def did_update_value(self, c, error):
		''' called after notify or read event 
			as cb does not support descriptors, notifiers are not used for analog chans but just read with waiting for result and chId is found in self.updating
		'''
		lodr = tls.query_lod(self.charFound, 
			lambda rw:rw['charis'].uuid==c.uuid and (rw[chID]==self.updating or self.updating==0))  # may find wrong anachan on notifying !!!
		if lodr:
			respCallback = lodr[0][chCallback]
		else:
			logger.warning('%s not found ' % c.uuid)
			respCallback=None
			lodr=[{chID:-1}]
		if respCallback is None:
			sval = ''.join('{:02x}'.format(x) for x in c.value)
			logger.info('Updated value: %s from %s' % (sval, c.uuid))
		else:
			respCallback(c)  # allready having chId from partial, lodr[0][chID])
		self.updating = 0
		
	def did_update_state(self):
		logger.info('update state:%s' % self.peripheral.state)
									
	def write_characteristic(self, chId, chData):
		''' write data to ble characteristic '''
		rec = tls.query_lod(self.charFound, lambda rw: rw[chID]==chId)
		c = rec[0]['charis']
		logger.debug('writing val to c:%s val:%s' % (c.uuid,''.join('{:02x}'.format(x) for x in chData)))
		rec[0]['periph'].write_characteristic_value(c, chData, c.properties & cb.CH_PROP_WRITE_WITHOUT_RESPONSE ==0)

	def read_characteristic(self, chId, waitReceived=False, timeout=10):
		""" did_update_value callback will receive result
		"""
		rec = tls.query_lod(self.charFound, lambda rw:rw[chID]==chId)
		self.updating = chId
		if rec:
			char = rec[0]['charis']
			rec[0]['periph'].read_characteristic_value(char)
			tick = time.clock()
			while waitReceived and self.updating>0 and time.clock()-tick <timeout:
				time.sleep(0.1)
			return rec[0]['charis'].value
		else:
			logger.warning('no recs reading')

	def setup_notification(self, chId):
		""" setup notification for characteristic that support it
		  returns True when successfull or allready notifying
		"""
		isin = tls.query_lod(self.charFound, lambda rw:rw[chID]==chId)
		c = isin[0]['charis']
		if c.notifying:
			return True
		p = isin[0]['periph']
		logger.info('setup notification for perf:%s (c:%d) with prop:%s notifying:%d' % (p.name,chId, MaskedPropMsg(cbPropMsg, c.properties), c.notifying))
		if c.properties & cb.CH_PROP_INDICATE:
			p.set_notify_value(c, True)
		elif c.properties & cb.CH_PROP_NOTIFY:
			p.set_notify_value(c, True)
		else:
			return False
		return True
		
	def setup_response(self, chId, respCallback=None):
		""" setup callback for value of characteristic
		"""
		isin = tls.query_lod(self.charFound, lambda rw:rw[chID]==chId)
		isin[0][chCallback] = partial(respCallback ,chId=chId)   # to be used in self.did_update_value
			
	def _exampleRespCallback(self, charis, chId=0):
		if charis:
			logger.info('chId:%d callback:%s' % (chId,charis.value))
		else:
			logger.error('no charis')
			return
		sval = ''.join('{:02x}'.format(x) for x in charis.value)
		logger.info('Updated %d:%s value: %s i.e. %s' % (chId,charis.uuid, sval, tls.bytes_to_int(charis.value, '<')))
		
			
def discover_BLE_characteristics(lodBLE):
	""" discover bluetooth le peripherals and their chracteristics
		expects lodBLE : a list of dictionaries defining what to be searched
		returns bleDelegate object
	"""
	cb.set_verbose(True)
	cb.reset()
	Delg = bleDelegate(lodBLE)
	cb.set_central_delegate(Delg)
	cb.scan_for_peripherals()
	logger.info('Waiting for callbacks state=%s' % (cb.get_state()))
	while not Delg.allFound():
		time.sleep(1)
	logger.info('found %d characteristics' % len(Delg.charFound))
	cb.stop_scan()
	return Delg
	
# some generic BLE UUID values (www.bluetooth.org)
SVRAUTOM = '1815'
CHRDIGIO = '2A56'
CHRANAIO = '2A58'
CHRTEMP  = '2A6E'
CHRHUM   = '2A6F'
CHRECO2  = '7397'
CHRTVOC  = '7398'

MOOSHI   =  None #'Mooshi'
IPHONE   = 'hj xFony'
HUELAMP  = None #'Hue Lamp'

if __name__ == "__main__":
	lod = list()
	if MOOSHI:
		PerfName = MOOSHI
		SERIN = 0
		SEROUT = 1
		lod.extend([{chPERF:PerfName, chID:i, chPUID:None, chCUID:None} for i in (SERIN, SEROUT)])
		lod[SERIN][chCUID]  = 'FFA1'
		lod[SEROUT][chCUID] = 'FFA2'
	if SVRAUTOM:
		PerfName = 'AIOS'
		lod.extend([{chPERF:PerfName, chID:i, chPUID:None, chCUID:ch} 
		  for i,ch in zip((0,1,2,3,4,5,6),(CHRDIGIO,CHRANAIO,CHRANAIO,CHRTEMP,CHRHUM,CHRECO2,CHRTVOC))])
		logger.info(lod)
	if IPHONE:
		lod.extend([{chPERF:IPHONE, chID:12, chPUID:None, chCUID:'9A37'}])
	if HUELAMP:
		lod.extend([{chPERF:HUELAMP, chID:13, chPUID:None, chCUID:'47A2'}])
	cbDelg = discover_BLE_characteristics(lod)
	
	for rec in cbDelg.lodCharist:
		print(rec)
	if MOOSHI:
		cbDelg.write_characteristic(SERIN, bytes([0x00, 0x01]))
		cbDelg.setup_response(SEROUT)
	if SVRAUTOM:
		cbDelg.setup_response(0, cbDelg._exampleRespCallback)  # CHRDIGIO
		cbDelg.setup_notification(0)
		cbDelg.setup_response(2, cbDelg._exampleRespCallback)  # CHRANAIO
		cbDelg.read_characteristic(2) 
		cbDelg.setup_response(3, cbDelg._exampleRespCallback)  # CHRTEMP
		cbDelg.read_characteristic(3)
		cbDelg.setup_response(4, cbDelg._exampleRespCallback)  # CHRHUM
		cbDelg.setup_notification(4)
	time.sleep(10)
	cb.reset()
	print('bye')
