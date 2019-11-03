''' fetching and buffering data from an AIOSclient '''

import logging,time

if '.' in __name__:  # imported from higher level package
	from . import tls
	#logger = tls.get_logger()
else:
	import lib.tls as tls	# gives error when called from main package also using tls
logger = tls.get_logger(__file__,logging.DEBUG)
from lib import AIOSclient
import lib.GATTclient as GATTclient

class datSource (object):
	def __init__(self, bufsize, updaterate):
		self.aios = AIOSclient.AIOSclient(nAnaChans=2)
		self.maxbuflen = bufsize
		self.funcs={}
		self.queue={0:[], 1:[]}

	def set_function(self,chan,func):
		if type(func) is str:
			nms = self.getFuncNames()
			logger.info('setting %s on chan %d' % (func,chan))
			func = next(fnc for fnc,nm in nms.items() if nm==func)
		self.funcs[chan]=func
		return func # chId
		
	def take_sample(self):
		for chan,chId in self.funcs.items():
			nm = self.aios.getName(chId)
			self.queue[chan].append(self.aios.getValue(chId))
			if len(self.queue[chan])>self.maxbuflen:
				self.queue[chan].pop(0)
			logger.debug('%s: len.queue=%s' % (nm,len(self.queue[chan])))
			#self.queue[chan].append(self.aios.getAnaVal(chan))
			#self.queue[chan].append(self.aios.getDigBit(chan))
	
	def actResults(self):
		rslt = [self.queue[chan][-1] for chan in self.funcs]
		return rslt

	def getFuncNames(self):
		names={}
		for chId in range(50):
			nm = self.aios.getName(chId)
			if nm:
				names[chId] = nm
		return names
		
	def close(self):
		self.aios.close()
		
if __name__=="__main__":
	datsrc = datSource(20, 0.1)
	datsrc.set_function(0, GATTclient.chTEMP)
	datsrc.set_function(1, GATTclient.chECO2)
	
	retimer = tls.RepeatTimer(2, datsrc.take_sample)
	retimer.start()
	'''
	for chan,chId in datsrc.funcs.items():
		datsrc.add_sample(chan)
		nm = datsrc.getFuncNames()[chId]
		logger.info('%s=%s' % (nm,datsrc.queue[chan]))
		time.sleep(0.5) 
	'''
	time.sleep(60)
	retimer.stop()
	logger.warning('bye')
	datsrc.aios.close()
