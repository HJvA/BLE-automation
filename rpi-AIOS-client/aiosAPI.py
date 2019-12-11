"""
	GATT client for Automation IO server
	using bluepy from http://ianharvey.github.io/bluepy-doc/index.html
"""
import time
import asyncio
from bluepy import btle

if __name__ == "__main__":
	import sys,os,logging
	sys.path.append(os.path.join(os.path.dirname(os.path.realpath(__file__)), '../..'))
	import lib.tls as tls
	logger = tls.get_logger(__file__, logging.DEBUG)
else:
	import lib.tls as tls
	logger = tls.get_logger()
	#import logging
	#logger = logging.getLogger()
from accessories.BLEAIOS.bluepyBase import bluepyDelegate,BAS_SVR,showChars

DEVADDRESS = "d8:59:5b:cd:11:0c"
AIOS_SVR = "00001815-0000-1000-8000-00805f9b34fb"  # automation-IO
ENV_SVR  = "6c2fe8e1-2498-420e-bab4-81823e7b0c03"  # environmental quantities

mdOUT = 0b00	# following GATT AIOS for dig bit modes
mdINP = 0b10
mdNOP = 0b11


chTEMP=1
chHUMI=2
chECO2=3
chTVOC=4
chDIGI    = 10		# func id for digitals
chANA1ST  = 11		# func id for first analog channel
chBAT =5

CHARS={}
CHARS[chANA1ST] = "00002a58-0000-1000-8000-00805f9b34fb"
CHARS[chDIGI] = "00002a56-0000-1000-8000-00805f9b34fb"
CHARS[chTEMP] = "00002a6e-0000-1000-8000-00805f9b34fb"
CHARS[chHUMI] = "00002a6f-0000-1000-8000-00805f9b34fb"
CHARS[chECO2] = "6c2fe8e1-2498-420e-bab4-81823e7b7397"
CHARS[chTVOC] = "6c2fe8e1-2498-420e-bab4-81823e7b7398"
CHARS[chBAT]  = "00002a19-0000-1000-8000-00805f9b34fb"


SCALES={chTEMP:100.0, chHUMI:100.0, chANA1ST:10000 }
NAMES ={chTEMP:'temperature', chHUMI:'humidity', chECO2:'CO2', chTVOC:'VOC', 
		chDIGI:'digitalIO', chBAT:'devBatteryLev', chANA1ST:'AnalogChan'}

class aiosDelegate(bluepyDelegate):
	def __init__(self, devAddress=DEVADDRESS, scales=SCALES, chInpBits={chDIGI:16}, loop=None):
		super().__init__(devAddress, scales, loop)
		self.chInpBits=chInpBits
		self.digmods=[]
		self.bitvals=[]
		if chInpBits:
			for chB,bitn in chInpBits.items():
				logger.info('set inp on %d for chId %d' % (bitn,chB))
				self.setDigMode(bitn, mdINP, False)
		self._sendDigBits()
		logger.info('dev %s state:%s' % (self.dev, self.dev.getState() if self.dev else None))

	def _CharId(self, charist, CharDef=CHARS):
		uuid = charist.uuid
		if uuid in CHARS.values():
			return next(chID for chID,chUUID in CharDef.items() if chUUID==uuid)
		return None

	def startChIdNotifyer(self, chId):
		if self.dev is None:
			logger.error('no ble device for notifying on %d' % chId)
			return
		if chId == chDIGI or chId>=chANA1ST:
			service = self.dev.getServiceByUUID(btle.UUID(AIOS_SVR))
			if chId>=chANA1ST:
				descr = self.dev.getDescriptors()  # also having the characteristics
				hand=-99
				for des in descr:
					if des.uuid == CHARS[chANA1ST]:  # it is an analog channel charist
						hand = des.handle
						chT=self.dev.getCharacteristics(hand-1,hand)[0]
					if des.handle == hand+2:  # look for presentation format
						chPresForm = des.read()
						chan=chPresForm[5]     # actual adc channel
						logger.debug('analog chan:%d hand:%d chPresForm:%s' % (chan,hand,tls.bytes_to_hex(chPresForm)))
						self.notifying[hand] = chANA1ST+chan
						if chId == chANA1ST+chan:
							self.startNotification(chT)
		elif chId in [chTEMP,chHUMI,chECO2,chTVOC]:
			service = self.dev.getServiceByUUID(btle.UUID(ENV_SVR))
		elif chId == chBAT:
			service = self.dev.getServiceByUUID(btle.UUID(BAS_SVR))
		if chId in CHARS:
			chT = service.getCharacteristics(btle.UUID(CHARS[chId]))
			self.startNotification(chT[0])

	def _extBitsLen(self, nbits):
		''' extend size of bit storage '''
		if len(self.digmods) < nbits:
			self.digmods.extend([mdNOP] * (nbits-len(self.digmods)))
		if len(self.bitvals) < nbits:
			self.bitvals.extend([False] * (nbits-len(self.bitvals)))	
				
	def _digiParse(self,digbits):
		digbits =bytearray(digbits)
		nbits = len(digbits)*4  # 4 bits per byte
		bno =[]
		self._extBitsLen(nbits)
		for bti in range(nbits):
			bit2 = digbits[bti >> 2] >> ((bti & 3)*2)
			if (bit2 & 2) == 0:
				self.digmods[bti] = mdOUT
				self.bitvals[bti] = True if bit2 & 1 else False
			elif self.digmods[bti] == mdINP:
				self.bitvals[bti] = True if bit2 & 1 else False
				if bit2 & 1:
					bno.append(bti)
		logger.info('digi parse:%s inp1:%s'  % ('.'.join('{:02x}'.format(x) for x in digbits),bno))
		return bno
		
	def getDigBit(self,bitnr):
		''' bits stored in little endian order i.e. low bits first '''
		self._extBitsLen(bitnr+1)
		return  self.bitvals[bitnr] 
		
	def _sendDigBits(self):
		nbits = len(self.digmods)
		if nbits<=0:
			return
		bitsbuf = bytearray((nbits >> 2) + (1 if (nbits & 3) else 0))
		for bt in range(nbits):
			bit2 = self.digmods[bt]
			if bit2 == mdOUT and self.bitvals[bt]:
				bit2 |= 1
			bitsbuf[bt >> 2] |= bit2 << ((bt & 3)*2)
		service = self.dev.getServiceByUUID(btle.UUID(AIOS_SVR))
		chT = service.getCharacteristics(btle.UUID(CHARS[chDIGI]))
		self.write(chT[0], bytes(bitsbuf))

	def setDigBit(self, bitnr, val, updateRemote=True):
		self._extBitsLen(bitnr+1)
		if self.digmods[bitnr] == mdNOP:
			self.setDigMode(bitnr, mdOUT, False)
		self.bitvals[bitnr] = True if val else False
		if updateRemote:
			self._sendDigBits()

	def setDigMode(self, bitnr, mode, updateRemote=True):
		self._extBitsLen(bitnr+1)
		if self.digmods[bitnr] != mode:
			self.digmods[bitnr] = mode
		if updateRemote:
			self._sendDigBits()

	async def receiveCharValue(self):
		""" consume received notification data """
		chId,val = await super().receiveCharValue()
		if chId == chDIGI:
			self._digiParse(val)  # get bitvals
			bits=[]
			for chB,bitn in self.chInpBits.items():
				if self.bitvals[bitn]:
					bits.extend([chB,1])
				logger.debug('digi:%d mask:%0x => %s' % (chB, 1 << self.chInpBits[chB], bits))
			if bits:
				return tuple(bits)
			return chId,None
		else:
			return chId,val

async def main():
	logger.info("Connecting...")
	aios = aiosDelegate(DEVADDRESS, masks={chDIGI:1 << 16})
	logger.info("getting notified in %s" % aios)
	if aios.dev:
		aios.startServiceNotifyers(aios.dev.getServiceByUUID(btle.UUID(ENV_SVR))) # activate environamental service
	aios.startChIdNotifyer(chDIGI)
	#aios.startChIdNotifyer(chANA1ST+3)  # activate 3rd analog channel
	
	await asyncio.gather( * aios.tasks() )

if __name__ == "__main__":	# testing 
	#aiosID = btle.UUID(AIOS_SVR)
	#aios = dev.getServiceByUUID(aiosID)
	#showChars(aios)

	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		logger.warning('leaving')

	#dev.disconnect()
	time.sleep(1)
	logger.warning('bye')