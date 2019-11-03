"""
	GATT client for Automation IO server
	using bluepy from http://ianharvey.github.io/bluepy-doc/index.html
"""
import time
import asyncio
from bluepy import btle

import tls
if __name__ == "__main__":
	logger = tls.get_logger(__file__)
else:
	logger = tls.get_logger()

DEVADDRESS = "d8:59:5b:cd:11:0c"
AIOS_SVR = "00001815-0000-1000-8000-00805f9b34fb"
ENV_SVR  = "6c2fe8e1-2498-420e-bab4-81823e7b0c03"
DEVINF_SVR="0000180a-0000-1000-8000-00805f9b34fb"

chTEMP=1
chHUMI=2
chECO2=3
chTVOC=4
chDIGI    = 10		# func id for digitals
chANA1ST  = 11		# func id for first analog channel

CHARS={}

CHARS[chANA1ST] = "00002a58-0000-1000-8000-00805f9b34fb"
CHARS[chDIGI] = "00002a56-0000-1000-8000-00805f9b34fb"
CHARS[chTEMP] = "00002a6e-0000-1000-8000-00805f9b34fb"
CHARS[chHUMI] = "00002a6f-0000-1000-8000-00805f9b34fb"
CHARS[chECO2] = "6c2fe8e1-2498-420e-bab4-81823e7b7397"
CHARS[chTVOC] = "6c2fe8e1-2498-420e-bab4-81823e7b7398"

SCALES={chTEMP:100.0, chHUMI:100.0 }
NAMES ={chTEMP:'temperature', chHUMI:'humidity'}

def CharId(uuid, CharDef=CHARS):
	return next(chID for chID,chUUID in CharDef.items() if chUUID==uuid)

def showChars(svr):
	logger.info('svr %s' % svr)
	for ch in svr.getCharacteristics():
		logger.info("ch %s %s" % (str(ch),ch.propertiesToString()))
		if ch.supportsRead():
			byts = ch.read()
			num = int.from_bytes(byts, byteorder='little', signed=False)
			logger.info("read %d:%s %s" % (ch.getHandle(),tls.bytes_to_hex(byts),num))
			
	
class aiosDelegate(btle.DefaultDelegate):
	def __init__(self, svrNotifyers=[], scales=SCALES):
		super().__init__()
		self.queue = asyncio.Queue()
		self.notifying = {}
		self.scales=scales
		for svr in svrNotifyers:
			if isinstance(svr, str):
				svr = dev.getServiceByUUID(btle.UUID(svr))
			for chT in svr.getCharacteristics(): 
				self.startNotification(chT)

	def handleNotification(self, cHandle, data):
		self.queue.put_nowait((cHandle,data))

	def startNotification(self, charist):
		''' sets charist on ble device to notification mode '''
		hand = charist.getHandle()
		chId = CharId(charist.uuid)
		if charist.properties & btle.Characteristic.props["NOTIFY"]:
			self.notifying[hand] = chId
			charist.peripheral.writeCharacteristic(hand+1, b"\x01\x00", withResponse=True)
			logger.info('starting notificatio on %s' % charist)
		else:
			logger.warning('NOTIFY not supported by:%s' % charist)
		if charist.supportsRead():
			val = charist.read()
			self.queue.put_nowait((hand,val))

	async def receiveCharValue(self):
		tup = await self.queue.get()
		chId = None
		if tup:
			val = tls.bytes_to_int(tup[1], '<')
			if tup[0] in self.notifying:
				chId = self.notifying[tup[0]]
		else:
			val = float('NaN')
		if chId in self.scales:
			val = float(val) / self.scales[chId]
		logger.info('chId:%s = %s' % (chId,val))
		self.queue.task_done()
		return chId,val

	async def awaitingNotifications(self):
		while True:
			dat = await self.receiveCharValue()

	async def servingNotifications(self, dev):
		while True:
			if dev.waitForNotifications(0.1):
				#await self.receiveCharValue()
				pass
			await asyncio.sleep(0.1)
		
	def tasks(self):
		''' background tasks receiving notifications from BLE device '''
		return [ asyncio.create_task(self.awaitingNotifications()),
					asyncio.create_task(self.servingNotifications(dev)) ]
	
async def main(dev):
	aios = aiosDelegate(svrNotifyers = [dev.getServiceByUUID(btle.UUID(ENV_SVR))])
	dev.withDelegate( aios )
	await asyncio.gather( * aios.tasks() )

if __name__ == "__main__":	# testing 

	logger.info("Connecting...")
	dev = btle.Peripheral(DEVADDRESS, btle.ADDR_TYPE_RANDOM) #  btle.ADDR_TYPE_PUBLIC)
	
	logger.info('dev %s iface:%s' % (dev,dev.iface))
	
	aiosID = btle.UUID(AIOS_SVR)
	aios = dev.getServiceByUUID(aiosID)
	
	descr = dev.getDescriptors()
	for des in descr:
		try:
			logger.debug('descr:%d:%s: %s' % (des.handle, des, tls.bytes_to_hex(des.read())))
		except btle.BTLEGattError as e:
			logger.warning('%s:%s:' % (des,e ))
	
	showChars(aios)
	#showChars(envs)
	
	logger.info("Services...")
	for svc in dev.services:
		logger.info(str(svc))
		time.sleep(0.1)
	
	logger.info("getting notified")
	asyncio.run(main(dev))
		
	dev.disconnect()
	logger.warning('bye')