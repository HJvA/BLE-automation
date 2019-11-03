#
import logging,time
#import ui
if '.' in __name__:  # imported from higher level package
	from . import tls
	#logger = tls.get_logger()
else:
	import sys
	sys.path.insert(0,'..')
	import lib.tls as tls	# gives error when called from main package also using tls
logger = tls.get_logger(__file__, logging.DEBUG)
from lib import cbBle as cb,GATTclient
#from pyploty.uiView_context import uiView_context

chDIGI    = 10		# func id for digitals
chANA1ST  = 11		# func id for first analog channel
mdOUT = 0b00	# following GATT AIOS
mdINP = 0b10
mdNOP = 0b11
anaDSCFACT = 10000

class AIOSclient (GATTclient.GATTclient):
	""" interfacing to device with BLE automation-IO profile
	"""
	bitvals = []
	digmods = []
	def __init__(self, *args, nAnaChans=3, **kwargs):
		""" 	"""
		self.nAnaChans = nAnaChans			
		super().__init__(*args, **kwargs)
			
		self.delg.setup_response(self.chofs, self._digCallback)
		self.readDigBits()
		for i in range(nAnaChans):
			GATTclient.scales[chANA1ST+i] = anaDSCFACT
		#	self.delg.setup_response(i+1+self.chofs, self._anaCallback)
	
	def getlod(self):
		''' override to add specific AIOS characteristic definitions to be discovered '''
		PerfName='AIOS'
		self.chofs = chDIGI
		lod = super().getlod()
		lod.extend([{cb.chPERF:PerfName, cb.chID:self.chofs, cb.chPUID:None, cb.chCUID:cb.CHRDIGIO}])
		for i in range(self.nAnaChans):
			lod.append({cb.chPERF:PerfName, cb.chID:i+chANA1ST, cb.chPUID:None, cb.chCUID:cb.CHRANAIO})
		return lod
	
	def _extBitsLen(self, nbits):
		if len(self.digmods) < nbits:
			self.digmods.extend([mdNOP] * (nbits-len(self.digmods)))
		if len(self.bitvals) < nbits:
			self.bitvals.extend([False] * (nbits-len(self.bitvals)))	
				
	def _digCallback(self,charact,chId=None):
		digbits =bytearray(charact.value)
		nbits = len(digbits)*4
		self._extBitsLen(nbits)
		for bti in range(nbits):
			bit2 = digbits[bti >> 2] >> ((bti & 3)*2)
			if (bit2 & 2) == 0:
				self.digmods[bti] = mdOUT
				self.bitvals[bti] = True if bit2 & 1 else False
			elif self.digmods[bti] == mdINP:
				self.bitvals[bti] = True if bit2 & 1 else False
		logger.info('dig receive:%s'  % '.'.join('{:02x}'.format(x) for x in charact.value))
			
	def readDigBits(self, waitReceived=True):
		self.delg.setup_notification(self.chofs)	# only effectuates first time
		self.delg.read_characteristic(self.chofs, waitReceived)
		
	def getDigBit(self,bitnr):
		''' bits stored in little endian order i.e. low bits first '''
		self._extBitsLen(bitnr)
		return  self.bitvals[bitnr] 
		
	def getDigMode(self,bitnr):
		'''  '''
		self._extBitsLen(bitnr)
		return self.digmods[bitnr] 
	
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
		self.delg.write_characteristic(self.chofs, bytes(bitsbuf))
	
	def setDigBit(self, bitnr, val, updateRemote=True):
		self._extBitsLen(bitnr)
		if self.digmods[bitnr] == mdNOP:
			self.setDigMode(bitnr, mdOUT, False)
		self.bitvals[bitnr] = True if val else False
		if updateRemote:
			self._sendDigBits()

	def setDigMode(self, bitnr, mode, updateRemote=True):
		self._extBitsLen(bitnr)
		if self.digmods[bitnr] != mode:
			self.digmods[bitnr] = mode
		if updateRemote:
			self._sendDigBits()
			
	def nDigBits(self):
		''' number of dig bits rounded to multiples of 4 '''
		nbits = len(self.bitvals)
		if nbits:
			return nbits
		self.readDigBits(waitReceived=True)
		return len(self.bitvals)
		
	def getAnaVal(self, Ach, waitReceived=True):
		return super().getValue(Ach+self.chofs+0, waitReceived)
	
	def getName(self, chId):
		if chId==self.chofs:
			return 'digitals'
		elif chId is None:
			return ''
		elif chId>=chANA1ST:
			if chId-chANA1ST <= self.nAnaChans:
				return 'analog A%d' % (chId-chANA1ST,)
			return 'na'
		else:
			return super().getName(chId)
			
	def getUnit(self, chId):
		return super().getUnit(chId)

if __name__=="__main__":
	aios = AIOSclient(nAnaChans=3)
	time.sleep(10)
	aios.setDigBit(19,1)
	print(aios.readDigBits())
	print(aios.getDigBit(19))
	bits = [aios.getDigBit(bitnr) for bitnr in range(aios.nDigBits())]
	print(bits)
	print(aios.getAnaVal(2))
	time.sleep(1)
	aios.setDigBit(19,0)
	logger.warning('bye')
	aios.delg.close()
