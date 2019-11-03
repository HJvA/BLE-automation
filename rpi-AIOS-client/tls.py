""" small general purpose helpers """

import datetime
import time
import logging
import os,re,sys
import threading

def bytes_to_int(data, endian='>'):
	"""Convert a bytearray into an integer, considering the first bit sign."""
	if endian=='<':
		data=bytearray(data)
		data.reverse()
	negative = data[0] & 0x80 > 0
	if negative:
		inverted = bytearray(~d % 256 for d in data)
		return -bytes_to_int(inverted) - 1
	encoded = ''.join(format(x, '02x') for x in data) 
	return int(encoded, 16)

def bytes_to_hex(data):
	return ''.join(format(x, '02x') for x in data)

def load_lod(lod, csv_fp, fields=['id','name']):
	''' get list of dict from csv file
	'''
	rdr = csv.DictReader(csv_fp, fields)
	lod.extend(rdr)

def query_lod(lod, filter=None, sort_keys=None):
	''' get list of dict filtered from lod
	pprint(query_lod(lod, sort_keys=('Priority', 'Year')))
	print len(query_lod(lod, lambda r:1997 <= int(r['Year']) <= 2002))
	print len(query_lod(lod, lambda r:int(r['Year'])==1998 and int(r['Priority']) > 2))
	'''
	if filter is not None:
		lod = (r for r in lod if filter(r))
	if sort_keys is not None:
		lod = sorted(lod, key=lambda r: [r[k] for k in sort_keys])
	else:
		lod = list(lod)
	return lod

def lookup_lod(lod, **kw):
	''' get first dict from lod with key value
	pprint(lookup_lod(lod, Row=1))
	pprint(lookup_lod(lod, Name='Aardvark'))
	'''
	i=0
	for row in lod:
		for k,v in kw.items():
			if row[k] != v:   
				i += 1
				break #next row
		else:
			return row,i
	return None,-1

def seconds_since_epoch(epoch = datetime.datetime.utcfromtimestamp(0), utcnow=datetime.datetime.utcnow()):
	''' time in s since 1970-1-1 midnight utc
	'''
	return (utcnow - epoch).total_seconds()


class RepeatTimer(object):
	def __init__(self, interval, function, *args, **kwargs):
		logging.info('setting up interval timer to run %s every %f seconds' % (function,interval))
		self._timer     = None
		self.interval   = interval
		self.function   = function
		self.args       = args
		self.kwargs     = kwargs
		self.is_running = False
		self.start()

	def _run(self):
		self.is_running = False
		self.start()
		self.function(*self.args, **self.kwargs)

	def start(self):
		if not self.is_running:
			self._timer = threading.Timer(self.interval, self._run)
			self._timer.start()
			self.is_running = True

	def stop(self):
		self._timer.cancel()
		self._timer.join() # hold main tread till realy finished
		self.is_running = False	

"""												
def set_logger(filename=None, format='%(levelname)-6s %(message)s', level=logging.NOTSET):
	'''obsolete
	'''
	formatter = logging.Formatter(format)
	if filename is None:
		hand = logging.StreamHandler() # console
	else:
		hand = logging.FileHandler(filename=filename, mode='w')
	hand.setLevel(level)
	hand.setFormatter(formatter)
	logger = logging.getLogger()
	logger.setLevel(level)
	[logger.removeHandler(h) for h in logger.handlers[::-1]]
	logger.addHandler(hand)
	return hand
"""	

def set_logger(logger, pyfile=None, levelConsole=logging.INFO, levelLogfile=logging.DEBUG):
	""" reset logger to desired config having several handlers :
	Console; logFile; errorLogFile"""
	[logger.removeHandler(h) for h in logger.handlers[::-1]] # handlers may persist between calls
	hand=logging.StreamHandler()
	hand.setLevel(levelConsole)
	logger.addHandler(hand)	# use console
	
	# always save errors to a file
	hand = logging.FileHandler(filename='error.log', mode='a')
	hand.setLevel(logging.ERROR)	# error and critical
	logger.addHandler(hand)
	
	reBASE=r"([^/]+)(\.\w+)$"
	base = re.search(reBASE,pyfile)
	if base:
		base=base.group(1)
	else:
		base=__name__
	logger.addHandler(logging.FileHandler(filename=base+'.log', mode='w', encoding='utf-8'))
	logger.setLevel(levelLogfile)
	if pyfile == "__main__":
		logger.critical("### running %s dd %s ###" % (__name__,time.strftime("%y%m%d %H:%M:%S")))
	return logger

def get_logger(pyfile=None, levelConsole=logging.INFO, levelLogfile=logging.DEBUG):
	''' creates a logger logging to both console and to a log file but with different levels
		pyfile=None   : called by package as sub module : logger from __main__ to be used
		pyfile=__file__ : called by main or testing module : create new logger
	  '''
	if pyfile is None or pyfile!=sys.modules['__main__'].__file__:
		return logging.getLogger(__name__)	# get logger from main program
	logger = logging.getLogger()
	return set_logger(logger, pyfile, levelConsole, levelLogfile)

	
if __name__ == "__main__":
	#set_logger(level=logging.INFO)
	#logging.basicConfig(level=logging.DEBUG, format='%(levelname)s - %(message)s')
	logger = get_logger(__file__)
	print('hand %d lev %s' % (len(logger.handlers), logger.handlers[0].flush()))
	#logger.handlers[0].setLevel(logging.DEBUG)
	#logger.setLevel(logging.DEBUG)
	logger.info('hallo wereld')
	logger.debug('debugging')
	logger.warning(os.getcwd())
	logger.error(os.getlogin())
	print('seconds since epoch : %s' % seconds_since_epoch())
	print('(0xff,0xff,0x7f) bytes_to_int = %0x' % bytes_to_int((0xff,0xff,0xfe),'<'))
	logging.shutdown()
