import logging

if '.' in __name__:  # imported from higher level package
	from . import tls
	logger = tls.get_logger()
else:
	import sys
	sys.path.insert(0,'..')
	import time
	import tls	# gives error when called from main package also using tls
	logger = tls.get_logger(__file__,logging.DEBUG)
import lib.cbBle as cb

chTEMP=1
chHUMI=2
chECO2=3
chTVOC=4
	
PerpheralName='AIOS'
names =  {chTEMP:'temperature',chHUMI:'humidity', chECO2:'eCO2', chTVOC:'TVOC'}
units =  {chTEMP:'°C',chHUMI:'%', chECO2:'ppm', chTVOC:'ppm'}
scales = {chTEMP:100.0,chHUMI:100.0}
# default 4 sensors defined to be discovered : chTEMP,chHUMI,chECO2,chTVOC
lod =[{cb.chPERF:PerpheralName, cb.chID:i, cb.chPUID:None, cb.chCUID:ch} 
		  for i,ch in zip(names.keys(),(cb.CHRTEMP,cb.CHRHUM,cb.CHRECO2,cb.CHRTVOC))]	

class GATTclient (object):
	def __init__(self, *args, **kwargs):
		lod = self.getlod()
		self.delg = cb.discover_BLE_characteristics(lod)
		self.vals = {}
		self.nvals={}
		for lodr in lod:
			self.delg.setup_response(lodr[cb.chID], self._RespCallback)
			self.vals[lodr[cb.chID]] = 0.0
			self.nvals[lodr[cb.chID]] =0

	def getlod(self):
		''' definition of expected characteristics to be discovered
		 may be overriden to give extra of them '''
		logger.info('lod :%d' % len(lod))
		return lod
				
	def _RespCallback(self,charis,chId):
		''' receive val from ble server
			add to average buffer
		'''
		num = tls.bytes_to_int(charis.value,'<')
		logger.debug('receiving %s = %s (%d)' % (self.getName(chId),charis.value,num))
		self.vals[chId] += num
		self.nvals[chId] += 1
		
	def getValue(self, chId, waitReceived=True):
		''' process values from aios server 
			waits for at least 1 value, and averages if multiple
		'''
		if waitReceived:
			self.delg.read_characteristic(chId, waitReceived=True)
		elif self.nvals[chId]==0:
			if self.delg.setup_notification(chId):
				pass # notifycation activated
			else:	# not supporting notification
				self.delg.read_characteristic(chId, waitReceived=False)
			time.sleep(0.1)  # allow time for req val to arrive
			
		val = float('NaN')
		if chId in self.nvals:
			if self.nvals[chId]:
				val = self.vals[chId] / self.nvals[chId]
			if chId in scales:
				val /= scales[chId]
		self.vals[chId] = 0.0
		self.nvals[chId] = 0
		return val
		
	def getName(self, chId):
		if chId in names:
			return names[chId]
		return None
		
	def getUnit(self, chId):
		if chId in units:
			return units[chId]
		return 'V' 
		
	def close(self):
		self.delg.close()

if __name__=="__main__":
	aios = GATTclient()
	time.sleep(10)
	for i in range(4):
		logger.info('%s=%s' % (aios.getName(i),aios.getValue(i)))
	time.sleep(1)
	logger.warning('bye')
	aios.delg.close()
