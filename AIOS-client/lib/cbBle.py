""" interface to pythonista version cb of 'CoreBluetooth'
		discovers specific devices (bluetooth peripherals) and their characteristics
		reads/writes from/to characteristics
"""
import cb
import time
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
		self.updated = False
		# self.respCallback=self._exampleRespCallback

	def close(self):
		"""
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
					logger.info("++ charist %d %s (notif:%d)-->%s" %
							(isin[0][chID], c.uuid, c.notifying, MaskedPropMsg(cbPropMsg, c.properties)))
					# debug.stop()
					self.charFound.append({chID:isin[0][chID], 'periph':perf, 'charis':c, chCallback:None})
				else:
					logger.info('not %s isin %s p:%s' % (c.uuid, self.ToBeFnd, perf.name))
					
	def did_write_value(self, c, error):
		logger.debug('Did write val to c:%s val:%s' % (c.uuid,''.join('{:02x}'.format(x) for x in c.value)))

	def did_update_value(self, c, error):
		''' called after notify or read event '''
		# respCallback = next((rec[chCallback] for rec in self.charFound if rec['charis'].uuid == c.uuid), None)
		lodr = tls.query_lod(self.charFound, lambda rw:rw['charis'].uuid==c.uuid)
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
			respCallback(c, lodr[0][chID])
		self.updated = True
		
	def did_update_state(self):
		logger.info('update state:%s' % self.peripheral.state)
									
	def write_characteristic(self, chId, chData):
		"""
		"""
		rec = tls.query_lod(self.charFound, lambda rw: rw[chID]==chId)
		c = rec[0]['charis']
		logger.debug('writing val to c:%s val:%s' % (c.uuid,''.join('{:02x}'.format(x) for x in chData)))
		rec[0]['periph'].write_characteristic_value(c, chData, c.properties & cb.CH_PROP_WRITE_WITHOUT_RESPONSE ==0)

	def read_characteristic(self, chId, waitReceived=False):
		""" response callback will receive result
		"""
		rec = tls.query_lod(self.charFound, lambda rw:rw[chID]==chId)
		self.updated = False
		if rec:
			rec[0]['periph'].read_characteristic_value(rec[0]['charis'])
			while waitReceived and not self.updated:
				time.sleep(0.1)
			return rec[0]['charis'].value
		else:
			logger.warning('no recs reading')

	def setup_response(self, chId, respCallback=None):
		""" setup notifyer or just get value of characteristic
		"""
		isin = tls.query_lod(self.charFound, lambda rw:rw[chID]==chId)
		p = isin[0]['periph']
		c = isin[0]['charis']
		logger.info('setup response for %s with prop:%s notifying:%d' % (p.name, MaskedPropMsg(cbPropMsg, c.properties), c.notifying))
		isin[0][chCallback] = respCallback  # ???
		if c.notifying:
			pass  # p.set_notify_value(c, True)
		elif c.properties & cb.CH_PROP_INDICATE:
			p.set_notify_value(c, True)
		elif c.properties & cb.CH_PROP_NOTIFY:
			p.set_notify_value(c, True)
					
	def _exampleRespCallback(self, charis):
		if charis:
			logger.info('callback:%s' % charis.value)
		else:
			logger.error('no charis')
			return
		sval = ''.join('{:02x}'.format(x) for x in charis.value)
		logger.info('Updated %s value: %s i.e. %s' % (charis.uuid, sval, tls.bytes_to_int(charis.value, '<')))
		
			
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
	cbDelg = discover_BLE_characteristics(lod)
	
	for rec in cbDelg.lodCharist:
		print(rec)
	if MOOSHI:
		cbDelg.write_characteristic(SERIN, bytes([0x00, 0x01]))
		cbDelg.setup_response(SEROUT)
	if SVRAUTOM:
		cbDelg.setup_response(3, cbDelg._exampleRespCallback)
		cbDelg.read_characteristic(0)
	time.sleep(10)
	cb.reset()
	print('bye')
