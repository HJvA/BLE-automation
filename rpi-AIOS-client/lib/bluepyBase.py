"""
	base interface to Bluetooth Low Energy device
	using bluepy from http://ianharvey.github.io/bluepy-doc/index.html
"""
import time
import asyncio
from bluepy import btle

import logging
logger = logging.getLogger(__name__)


def showChars(svr):
	''' lists all characteristics from a service '''
	logger.info('svr %s uuid %s' % (svr,svr.uuid))
	for ch in svr.getCharacteristics():
		logger.info("ch %s %s %s %s" % (str(ch),ch.propertiesToString(),ch.getHandle(),ch.uuid))
		if ch.supportsRead():
			try:
				byts = ch.read()
				num = int.from_bytes(byts, byteorder='little', signed=False)
				logger.info("read:%s => %s" % (byts,num))
			except Exception as ex:
				logger.info('exception reading:%s len=%d' % (ex,len(byts)))

class bluepyDelegate(btle.DefaultDelegate):
	""" handling notifications asynchronously 
		must either be created in async co-routine or have loop supplied """
	def __init__(self, devAddress, scales={}, loop=None):
		super().__init__()
		logger.info("connecting to BLE device:%s scaling:%s on %s" % (devAddress,scales,loop))
		try:
			self.dev = btle.Peripheral(devAddress)  #, btle.ADDR_TYPE_RANDOM)  #, btle.ADDR_TYPE_PUBLIC
			#time.sleep(1)
			self.dev.withDelegate( self )
			#time.sleep(1)
		except btle.BTLEDisconnectError as e:
			logger.error('unable to connect ble device : %s' % e)
			self.dev = None
		self.queue = asyncio.Queue(loop=loop)
		self.notifying = {}
		self.scales=scales
		if self.dev:
			stat = self.dev.getState()
			for svc in self.dev.services:  # internally populate services
				#if btle.UUID(AIOS_SVR) == svc.uuid:
				#	aios=svc
				logger.info('stat:%s service:%s' % (stat,str(svc)))
		
	def handleNotification(self, cHandle, data):
		""" callback getting notified by bluepy """
		self.queue.put_nowait((cHandle,data))

	def startServiceNotifyers(self, service):
		""" start notification on all characteristics of a service """
		for chT in service.getCharacteristics(): 
			self.startNotification(chT)

	def _CharId(self, charist):
		""" virtual ; returns unique id of chracteristic (also when multiple chars of same type are there) """
		return charist.getHandle()

	def startNotification(self, charist):
		""" sets charist on ble device to notification mode """
		hand = charist.getHandle()
		if charist.properties & btle.Characteristic.props["NOTIFY"]:
			if hand in self.notifying:
				chId = self.notifying[hand]
			else:
				chId = self._CharId(charist)
				self.notifying[hand] = chId
			charist.peripheral.writeCharacteristic(hand+1, b"\x01\x00", withResponse=True) # cccd on hand+1
			logger.info('starting notificatio on (%d) %s hand=%d' % (chId,charist,hand))
		else:
			logger.warning('NOTIFY not supported by:%s on %s' % (hand,charist))
		val = self.read(charist)
	
	def hasCharValue(self):
		return self.queue.qsize()
		
	async def receiveCharValue(self):
		""" consume received notification data """
		tup = await self.queue.get()
		chId = None
		if tup:
			if tup[0] in self.notifying:
				chId = self.notifying[tup[0]]
			if tup[1] and len(tup[1])<=4:
				val = int.from_bytes(tup[1], 'little') #  tls.bytes_to_int(tup[1], '<', False)
			else:
				val = tup[1]  # keep bytes for digitals
		else:
			val = float('NaN')
		if chId in self.scales:
			val = float(val) / self.scales[chId]
		logger.debug('ble chId:%s = %s' % (chId,val))
		self.queue.task_done()
		return chId,val

	def read(self, charist):
		""" read value from characteristic on device put result also in async queue """
		if charist.supportsRead():
			try:
				val = charist.read()
			except btle.BTLEGattError as e:
				logger.warning ('bluepy error on read charist :%s' % e)
				val = None
			if self.queue:
				hand = charist.getHandle()
				self.queue.put_nowait((hand,val))
			return val
			
	def write(self, charist, data):
		#if charist.supportsWrite():
		try:
			charist.write(data)
		except btle.BTLEInternalError as e:
			logger.warning ('bluepy error on write charist :%s' % e)

	async def awaitingNotifications(self):
		""" keep consuming received notifications """
		logger.info('awaiting aios notifications')
		while self.dev is not None:
			dat = await self.receiveCharValue()

	async def _recoverConnection(self):
		""" try to reconnect and restore notifying state, when BLE connection got lost """
		try:
			logger.error('BLE disconnected adr:%s adrtp:%s' % (self.dev.addr,self.dev.addrType))
			await asyncio.sleep(5)
			self.dev.connect(self.dev.addr, self.dev.addrType, self.dev.iface)
			await asyncio.sleep(0.5)
			logger.info('BLE reconnected : %s' % self.dev.getState())
			if self.dev.getState():
				for hnd,chId in self.notifying.items():
					logger.debug('getting charist %d at hnd %d' % (chId,hnd))
					charist = self.dev.getCharacteristics(hnd-1,hnd)
					if charist:
						self.startNotification(charist[0])
		except btle.BTLEDisconnectError as e:
			logger.error("error recovering BLE connection:%s" % e)
		except Exception as e:
			logger.error("unrecoverable BLE error :%s" % e)
			self.dev._helper = None
			await asyncio.sleep(10)
			
	async def servingNotifications(self):
		""" keep polling bluepy for received notifications """
		logger.info('serving aios notifications on %s' % self.dev)
		while self.dev is not None:
			try:
				if self.dev._helper is not None:
					if self.dev.waitForNotifications(0.1):
						#await self.receiveCharValue()
						pass
				else:
					await self._recoverConnection()
			except (btle.BTLEDisconnectError, btle.BTLEInternalError) as e:
				await self._recoverConnection()
			await asyncio.sleep(0.1)
		
	def tasks(self):
		''' background tasks receiving notifications from BLE device '''
		return [ asyncio.create_task(self.awaitingNotifications()),
					asyncio.create_task(self.servingNotifications()) ]
	

if __name__ == "__main__":	# 
	""" testing : call it with python3 accessories/BLEAIOS/bluepyBase.py | tee bluepyBase.log
	"""
	from tls import get_logger
	logger = get_logger(__file__,logging.DEBUG)
	
	#some example GATT services
	DEVINF_SVR= "180a"  #"0000180a-0000-1000-8000-00805f9b34fb"  # device info
	BAS_SVR   = "180f"  #"0000180f-0000-1000-8000-00805f9b34fb"  # battery level
	AIOS_SVR  = 0x1815  # "1815"
	TEMP_CHR  = "2a6e"
	DIG_CHR   = "2a56"
	#logging.basicConfig(level=logging.DEBUG)   #, filename="bluepyBase.log")

	DEVADDRESS = "C9:04:5E:8D:26:97" #  "d8:59:5b:cd:11:0c"	# find your device e.g. using bluetoothctl using the scan on command
	
	async def main(charsNotifying):
		logger.info("Connecting...")
		delg = bluepyDelegate(DEVADDRESS)
		aios = None
		#dev = btle.Peripheral(DEVADDRESS, btle.ADDR_TYPE_RANDOM) #  btle.ADDR_TYPE_PUBLIC)
		if not delg or not delg.dev:
			logger.warning("BLE error unexpectedly leaving")
			return
		logger.info('dev %s state:%s iface:%s' % (delg.dev, delg.dev.getState(), delg.dev.iface) )
		logger.info("Services...")
		for svc in delg.dev.services:
			stat = delg.dev.getState()
			if btle.UUID(AIOS_SVR) == svc.uuid:
				aios=svc
			logger.info('stat:%s service:%s' % (stat,str(svc)))
			await asyncio.sleep(0.1)
			
			chars = svc.getCharacteristics()
			for char in chars:
				props = char.propertiesToString()
				logger.info('char:%s prop:%s' % (char.uuid, props))
				for id in charsNotifying:
					if btle.UUID(id) == char.uuid:
						logger.info('starting notif on %s' % char)
						delg.startNotification(char) 
				if char.uuid == btle.UUID(DIG_CHR):
					bts = delg.read(char)
					logger.debug('reading digitals:%s' % bts)
		aios = delg.dev.getServiceByUUID(btle.UUID(AIOS_SVR))
		await asyncio.sleep(0.1)
		if aios and delg.dev and delg.dev.getState():
			showChars(aios)
			descr = aios.getDescriptors()
			for des in descr:
				try:
					logger.info('descr:%d:%s: %s' % (des.handle, des, des.read()))
				except btle.BTLEGattError as e:
					logger.warning('error GattDescr:%s:%s:' % (des,e ))
				"""
			if servNotifying:
				for srv in servNotifying:
					try:
						delg.startServiceNotifyers(delg.dev.getServiceByUUID(btle.UUID(srv)))
					except btle.BTLEGattError as e:
						logger.warning('error GattNotif:%s:%s:' % (srv,e ))
					except btle.BTLEDisconnectError as e:
						logger.warning('error BLE discon:%s:%s:' % (srv,e ))
				"""
		#dev.withDelegate( delg )
		logger.info('waiting for notifications dev state=%s' % delg.dev.getState())
		try:
			await asyncio.gather( * delg.tasks() )
		except KeyboardInterrupt:
			logger.warning('leaving')
		finally:
			delg.dev.disconnect()

	
	logger.info("getting notified")
	try:
		charist = [DIG_CHR]   #,TEMP_CHR]
		asyncio.run(main(charist))
	except Exception as e:
		logger.error('exception:%s' % e)
	logger.warning('bye')