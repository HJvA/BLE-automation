""" main user entry point for BLE Automation-IO client """
import ui
import dialogs
import time
import logging
import lib.tls as tls
from pyploty.uiView_context import uiView_context, vwDigitals
from pyploty.plottls import rect_area
from pyploty.xy_plot import xy_graph
import datSource
from lib.AIOSclient import mdINP,mdOUT

def take_sample(datsrc):
	''' take sample from datsrc and draw result on actPage '''
	global actPage
	datsrc.take_sample()
	if actPage is vrslt:
		rslt = datsrc.actResults()
		if rslt:
			logger.info('setting rslt:%s' % rslt)
			update_results(rslt)
			bits = [datsrc.aios.getDigBit(bitnr) for bitnr in range(datsrc.aios.nDigBits())]
			modes = [datsrc.aios.getDigMode(bitnr) for bitnr in range(datsrc.aios.nDigBits())]
			if bits:
				vrslt['digitals'].bitvals = bits
				vrslt['digitals'].modes = modes
				vrslt['digitals'].set_needs_display()  # uiView_context.vwDigitals.draw invoked
	if actPage is vchart:
		rslt = datsrc.queue	# take it all
		vchart.results = rslt
		vchart.set_needs_display()

	
def update_results(results):
	''' feeding results from meter to vrslt page '''
	for i,rslt in enumerate(results):
		chn = i+1
		vrslt['rsltVal%d' % chn].text ='%4f'  % rslt
		vrslt['rsltBar%d' % chn].value = rslt/float(vseti['target%d' % chn].text)

def show_page(page_idx, pages):
	logger.info('showing page %s/%d' % (page_idx,len(vmain.subviews)))
	global actPage 
	if actPage is None: # first time
		for pg in pages:
			pg.hidden=True
			pg.y=60
			pg.flex='WH'
			vmain.add_subview(pg)
		set_func(1)
		set_func(2)
	else:
		actPage.hidden=True
	actPage = pages[page_idx]
	#vmain.add_subview(actPage)
	actPage.hidden=False
	#if page_idx==2:
	#	grctx.draw_recorded()
	
def set_func(chan, nmFunc=None):
	vseti['function%d' % chan].title = nmFunc
	vseti['target%d' % chan].text = '100'
	if nmFunc:
		chId = datsrc.set_function(chan-1, nmFunc)
		vrslt['unit%d' % chan].text = datsrc.aios.getUnit(chId)
		vrslt['rsltLbl%d' % chan].text = nmFunc

def get_func(chan):
	return vseti['function%d' % chan].title, vrslt['unit%d' % chan].text, vseti['target%d' % chan].text

def ask_function(chan):
	lds =ui.ListDataSource([{'title':tm} for tm in datsrc.getFuncNames().values()]	)
	sel =dialogs.list_dialog('select function',lds.items)
	if sel:
		trg = float(vseti['target%d' % chan].text)
		set_func(chan, sel['title'])
		logger.info('setup func:%s for %d with %s' % (sel['title'],chan,trg))
		return sel['title']
		
##******* event handlers ********##
def func1act(sender):
	''' set picked item to combo '''
	sender.title = ask_function(1)

def func2act(sender):
	sender.title = ask_function(2)

def selPageAct(sender):
	''' handle event changing tab '''
	page = sender.selected_index
	show_page(page,(vseti,vrslt,vchart))

def ask_selDigital(deflt):
	lds =ui.ListDataSource([{'title':'pin nr %d' % btnr} for btnr in range(datsrc.aios.nDigBits())]	)
	sel = dialogs.list_dialog('select pin nr',lds.items)
	return sel['title'] if sel else 'select pin number'

def selDigital(sender):
	sender.title = ask_selDigital(sender.title)
	bitnr = int(sender.title[7:])
	vrslt['bitval'].value = datsrc.aios.getDigBit(bitnr)
	vrslt['bitmode'].value = (datsrc.aios.getDigMode(bitnr)==mdOUT)
	logger.info('get bit %s beeing %s' % (bitnr, vrslt['bitval'].value))
	
def switchDigital(sender):
	bitnr = int(vrslt['selDigital'].title[7:])
	logger.info('switch pin %s to %s' % (bitnr,sender.value))
	datsrc.aios.setDigBit(bitnr, sender.value)

def modeDigital(sender):
	bitnr = int(vrslt['selDigital'].title[7:])
	bitval = vrslt['bitval'].value
	logger.info('mode pin %s to %s' % (bitnr,sender.value))
	datsrc.aios.setDigMode(bitnr, mdOUT if sender.value else mdINP)

##******* page views *******##
class vwMain(ui.View):
	def did_load(self):
		global datsrc
		datsrc = datSource.datSource(20, 0.1)
		self.retimer = tls.RepeatTimer(2, take_sample, datsrc=datsrc)
		self.retimer.start()
		self['screenShot'].action = self.screenshot_action
	def will_close(self):
		print('closing')
		self.retimer.stop()
		time.sleep(1)
		datsrc.close()
	def screenshot_action(self,sender):
		with ui.ImageContext(self.width, self.height) as c:
			self.draw_snapshot()
			#c.get_image().show()
			filename = actPage.name+'_scrn_{}.png'.format(time.strftime("%d-%H:%M:%S", time.localtime()))
			logger.info('screenshot '+filename)
			with open(filename, 'wb') as out_file:
				out_file.write(c.get_image().to_png()) 

class vwSettings(ui.View):
	def did_load(self):
		self.background_color = 'beige'
		self.bg_color = 'white'

class vwResults(ui.View):
	def did_load(self):
		pass
		
class vwChart(ui.View):
	results={}  # fed in by :..
	def did_load(self):
		self.content_mode = ui.CONTENT_CENTER
		self.background_color = 'beige'
		self.bg_color = 'white'
	def layout(self):
		world= rect_area(*self.bounds).sub_area(0.1,0.05,0.8,0.85)
		logger.info('changing view size :%s' % world)
		ctx = uiView_context(self, world_area=world, user_area=rect_area(0,0,10,30))
		self.grctx ={0:ctx, 1:ctx}  # each channel has its own context i.e. left,right yaxis
	def draw(self):
		if self.results and self.results[0]: # and self.results[1]:
			grph = xy_graph(self.grctx)
			self.grctx[0].clear()
			for ch,curv in self.results.items():
				if curv:
					xs = list(range(len(curv)))
					logger.info('chan:%s xs:%s ys:%s' % (ch,xs,curv))
					grph.add_plot(xs, curv, ctxId=ch, color=(ch, 0.00, 1.00), name='%s' % get_func(ch+1)[0])
				else:
					logger.debug('no curv for ch:%s' % ch)
				grph.draw(autoscale=True,ctxId=ch)
		else:
			logger.debug('no chart result')

		
##****** main ******##
logger = tls.get_logger(__file__, logging.DEBUG)

actPage=None

vseti = ui.load_view('uiSettings.pyui')
vrslt = ui.load_view('uiResults.pyui')
vchart = ui.load_view('uiChart.pyui')
vmain = ui.load_view()

show_page(0,(vseti,vrslt,vchart))

if min(ui.get_screen_size()) >= 768:	# iPad
	vmain.frame = (0, 0, 500, 600)
	vmain.present('sheet')
else:			# iPhone
	vmain.present(orientations=['portrait'])

time.sleep(60)
logger.warning('bye')
