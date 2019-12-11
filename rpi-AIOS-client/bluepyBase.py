"""
	base interface to Bluetooth Low Energy device
	using bluepy from http://ianharvey.github.io/bluepy-doc/index.html
"""
import time
import asyncio
from bluepy import btle

if __name__ == "__main__":
	import logging
	logger = logging.getLogger()
else:
	import lib.tls as tls
	logger = tls.get_logger()

DEVINF_SVR="0000180a-0000-1000-8000-00805f9b34fb"  # device info
BAS_SVR   ="0000180f-0000-1000-8000-00805f9b34fb"		# battery level

def showChars(svr):
	''' lists all characteristics from a service '''
	logger.info('svr %s uuid %s' % (svr,svr.uuid))
	for ch in svr.getCharacteristics():
		logger.info("ch %s %s %s" % (str(ch),ch.propertiesToString(),ch.uuid))
		if ch.supportsRead():
			byts = ch.read()
			num = int.from_bytes(byts, byteorder='little', signed=False)
			logger.info("read %d:%s %s" % (ch.getHandle(),byts,num))

class bluepyDelegate(btle.DefaultDelegate):
	""" handling notifications asynchronously 
		must either be created in async co-routine or have loop supplied """
	def __init__(self, devAddress, scales={}, loop=None):
		super().__init__()
		try:
			self.dev = btle.Peripheral(devAddress, btle.ADDR_TYPE_RANDOM)
			self.dev.withDelegate( self )
		except btle.BTLEDisconnectError as e:
			logger.error('unable to connect ble device : %s' % e)
			self.dev = None
		self.queue = asyncio.Queue(loop=loop)
		self.notifying = {}
		self.scales=scales
		
	def handleNotification(self, cHandle, data):
		""" callback getting notified by bluepy """
		self.queue.put_nowait((cHandle,data))

	def startServiceNotifyers(self, service):
		for chT in service.getCharacteristics(): 
			self.startNotification(chT)
	
			
	def _CharId(self, charist):
		return charist.getHandle()
		if uuid in CHARS.values():
			return next(chID for chID,chUUID in CharDef.items() if chUUID==uuid)
		return None

	def startNotification(self, charist):
		''' sets charist on ble device to notification mode '''
		hand = charist.getHandle()
		if charist.properties & btle.Characteristic.props["NOTIFY"]:
			if hand in self.notifying:
				chId = self.notifying[hand]
			else:
				chId = self._CharId(charist)
				self.notifying[hand] = chId
			charist.peripheral.writeCharacteristic(hand+1, b"\x01\x00", withResponse=True)
			logger.info('starting notificatio on (%d) %s' % (chId,charist))
		else:
			logger.warning('NOTIFY not supported by:%s on %s' % (hand,charist))
		val = self.read(charist)

	async def receiveCharValue(self):
		""" consume received notification data """
		tup = await self.queue.get()
		chId = None
		if tup:
			if tup[0] in self.notifying:
				chId = self.notifying[tup[0]]
			if len(tup[1])<=4:
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
		''' read value from characteristic on device put result also in async queue '''
		if charist.supportsRead():
			val = charist.read()
			if self.queue:
				hand = charist.getHandle()
				self.queue.put_nowait((hand,val))
			return val
			
	def write(self, charist, data):
		#if charist.supportsWrite():
		charist.write(data)

	async def awaitingNotifications(self):
		""" keep consuming received notifications """
		while self.dev is not None:
			dat = await self.receiveCharValue()

	async def _recoverConnection(self):
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
	

if __name__ == "__main__":	# testing 
	DEVADDRESS = "d8:59:5b:cd:11:0c"
	
	async def main(servNotifying):
		logger.info("Connecting...")
		delg = bluepyDelegate(DEVADDRESS)
		#dev = btle.Peripheral(DEVADDRESS, btle.ADDR_TYPE_RANDOM) #  btle.ADDR_TYPE_PUBLIC)
	
		logger.info('dev %s iface:%s' % (delg.dev,delg.dev.iface))
	
		logger.info("Services...")
		for svc in delg.dev.services:
			logger.info(str(svc))
			time.sleep(0.1)
			showChars(svc)
		time.sleep(5)
		
		descr = delg.dev.getDescriptors()
		for des in descr:
			try:
				logger.debug('descr:%d:%s: %s' % (des.handle, des, des.read()))
			except btle.BTLEGattError as e:
				logger.warning('%s:%s:' % (des,e ))
		
			if servNotifying:
				for srv in servNotifying:
					delg.startServiceNotifyers(delg.dev.getServiceByUUID(btle.UUID(srv)))
		#dev.withDelegate( delg )
		
		try:
			await asyncio.gather( * delg.tasks() )
		except KeyboardInterrupt:
			logger.warning('leaving')
		finally:
			delg.dev.disconnect()

	
	logger.info("getting notified")
	asyncio.run(main([BAS_SVR]))
	
	logger.warning('bye')