''' inspired by Mats Klingberg 
'''
import math
import canvas
if '.' not in __name__:  # =="__main__" or imported from same package
	import sys
	sys.path.append('..')
from pyploty import plottls
from lib import tls
logger = tls.get_logger(__file__)

class xy_graph(object):
	def __init__(self, context, hold=True, grid=True):
		''' context= canvas_context or uiView_context object having world set to a drawing area  
			each yaxis side may have differen context '''
		if type(context) is dict:
			self.context = context
		else:
			self.context = {0:context} # graph to have different context for left and right yaxis
		self.hold=hold
		self.grid=grid
				
		self.axis_color = (0.40, 0.40, 0.40)
		self.datasets = []
		for id,ctx in self.context.items():
			self.set_bounds(*ctx.user.frame(), ctxId=id)
		
	def set_bounds(self, xmin,ymin, xsize, ysize, ctxId=0):		
		'''
		Set x- and y-axis user coord limits.
		Will also recalculate tick positions.
		'''
		if xsize==0 or ysize==0 or math.isinf(xmin)  or math.isinf(ymin):
			return 
		wrld_frame = self.context[ctxId].world
		user_frame = plottls.rect_area(xmin,ymin, xsize, ysize)
		logger.info('scaling graph wrld:%s user:%s' % (wrld_frame.frame(),user_frame.frame()))
		#self.innerbox = plottls.canvas_context(wrld_frame, user_frame)
		self.context[ctxId].user.set_origin(xmin,ymin)
		self.context[ctxId].user.set_size(xsize, ysize)
		xlim = (xmin, xmin+xsize)
		ylim = (ymin, ymin+ysize)
		if ctxId==0:
			self.xticks = _calc_ticks(xlim)
			#self.yticks = _calc_ticks(ylim,ctxId)
		self.rescaling=True

	def adjust_margins(self):	
		'''Calculate figure margins that allows tick labels to fit.'''
		xmarg = [0,0]
		ymarg = [0,0]
		# Process the x-axis tick labels.
		# TODO: The x margin is always set to half the width of the first and
		#       last tick label, respectively. This is only necessary if the
		#       labels are at the limits, otherwise the margins could be reduced.
		ws = []
		hs = []
		for t in self.xticks:
			_, w, h = self.get_ticklabel(t,0)
			ws.append(w)
			hs.append(h)
		xmarg[0] = ws[1]/2 
		xmarg[1] = ws[-1]/2 
		ymarg[0] = max(hs) 
		ymarg[1] = 0
		
		# Process y-axis tick labels.
		# The margins from the x-labels above are extended as necessary.
		# TODO: Similarly as for the x-labels, the margins are too high if the
		#       if the labels arn't at the limits.
		ws = []
		hs = []
		yticks = _calc_ticks((self.user.ymin, self.user.ymin+self.user.ysize),0)
		for t in yticks:
			_, w, h = self.get_ticklabel(t,0)
			ws.append(w)
			hs.append(h)
		xmarg[0] = max(max(ws)*1.1, xmarg[0])
		xmarg[1] = max(0, xmarg[1])
		ymarg[0] = max(hs[0]/2 , ymarg[0])
		ymarg[1] = max(hs[-1]/2 , ymarg[1])
		
	def draw_axis(self,ctxId=0):
		'''Draw axes and ticks.'''
		#canvas.set_stroke_color(*self.axis_color)
		self.context[ctxId].set_draw_color(self.axis_color)
		self.context[ctxId].draw_rect(*self.context[ctxId].user.frame())
		self.draw_ticks(ctxId)
		self.rescaling=False
		
	def get_ticklabel(self,tick,ctxId):
		'''Format a tick label.'''
		lbl = '%.3g' % tick
		w,h = self.context[ctxId].get_text_size(lbl)
		return lbl, math.fabs(w), math.fabs(h)

	def axSide(self,ctxId):
		''' left=1, right=-1 '''
		if ctxId & 1:
			return -1
		return 1	

	def draw_legend(self, datset):
		ctxId = datset['ctx']
		w,h = self.context[ctxId].user.get_size()
		x,y = self.context[ctxId].user.get_origin()
		if datset['name'] and datset['ctx']==ctxId:
			if self.axSide(ctxId)>0:
				self.context[ctxId].draw_text(datset['name'], x, y+h)
			else:
				tw,th = self.context[ctxId].get_text_size(datset['name'])
				self.context[ctxId].draw_text(datset['name'], x+w-tw, y+h)

	def draw_ticks(self,ctxId=0):	
		'''Draw ticks/grid and labels.'''
		w,h = self.context[ctxId].user.get_size()
		x,y = self.context[ctxId].user.get_origin()
		side=self.axSide(ctxId)
		yticks = _calc_ticks((y, y+h))
		for t in self.xticks:
			if side>0:	# only take x axis from left side		
				if self.grid:
					self.context[ctxId].draw_dashed_line(t, y, t, y+h)
				else:
					self.context[ctxId].draw_line(t, y, t, y+h/100)
					self.context[ctxId].draw_line(t, y+h, t, y+h - h/100)
				lbl, lw, lh = self.get_ticklabel(t,ctxId)
				self.context[ctxId].draw_text(lbl, t-lw/2, y-lh)
		
		for t in yticks:			
			if self.grid and side>0:
				self.context[ctxId].draw_dashed_line(x, t, x+w, t)
			else:
				self.context[ctxId].draw_line(x, t, x+w/100, t)
				self.context[ctxId].draw_line(x+w, t, x+w - w/100, t)
			lbl, lw, lh = self.get_ticklabel(t, ctxId)
			if side>0:
				self.context[ctxId].draw_text(lbl, x-lw-w*0.01, t-lh/3)
			else:
				self.context[ctxId].draw_text(lbl, x+w*1.01, t-lh/3)

	def add_plot(self, x, y, ctxId=0, color=(0.00, 0.00, 0.00), name=None):		
		'''Add plot data.'''	
		plotdata = {'xdata': x, 'ydata': y, 'ctx':ctxId, 'color': color, 'name':name}
		if self.hold:
			self.datasets.append(plotdata)
		else:
			self.datasets = [plotdata]
		
	def draw(self, autoscale=True, ctxId=0):		
		''' draw entire xy_graph as defined by above methods
			canvas specific : override for other target surface			
		'''
		if autoscale:
			xmin,xmax, ymin,ymax = _calc_limits(self.datasets,ctxId)
			self.set_bounds(xmin,ymin, xmax-xmin,ymax-ymin, ctxId)
		# All drawing operations are enclose within calls to 'begin_updates and
		# 'end_updates' to improve performance.
		self.context[ctxId].record_drawing()
		#canvas.begin_updates()
				
		#self.adjust_margins()
		if self.rescaling or not autoscale:
			self.draw_axis(ctxId)
		
		if not self.datasets:
			# No data to plot but draw axis
			self.context[ctxId].draw_recorded()
			logger.warning('no chart data')
			return
		
		# Clip plot lines to wihin the axes.
		self.context[ctxId].draw_rect(*self.context[ctxId].user.frame())
		#canvas.clip()

		# Plot data as a path with lines between each point.
		
		self.context[ctxId].set_emphasis(2)
		for ds in self.datasets:
			self.draw_legend(ds)
			if ds['ydata'] and ds['ctx']==ctxId:
				if ds['xdata']:
					xdat=ds['xdata']
				logger.debug('new plot xlen=%d, ylen=%d, col=%s' % (len(xdat),len(ds['ydata']),ds['color']))
				self.context[ctxId].set_draw_color(ds['color'])
				self.context[ctxId].move_to(xdat[0], ds['ydata'][0])
				for i in range(1, len(xdat)):
					self.context[ctxId].add_line(xdat[i], ds['ydata'][i])
				self.context[ctxId].draw_recorded()	# color might change		
		# Draw all content
		self.context[ctxId].draw_recorded()
		#canvas.end_updates()
		self.context[ctxId].set_emphasis(1)
		
							
		
def _calc_ticks(lim):	
	'''Return reasonable tick positions given x and y limits.
	'''	
	# Make the tick spacing 10**d, 2*10**d or 5*10**d, where d is selected to
	# give about n0 ticks along each axis. The chosen spacin is the
	# logarithmically closest to the exact value (d0).
	n0 = 5
	d0 = (lim[1] - lim[0]) / (n0 + 1)

	# Pick the logarithmically closest from 1,2 or 5
	fd, id = math.modf(math.log10(d0))
	if fd < 0:
		id = id - 1
		fd = fd + 1
	if fd <= 0.15:
		d = 10**id
	elif fd <= 0.5:
		d = 2*10**id
	elif fd <= 0.85:
		d = 5*10**id
	else:
		d = 10**(id+1)
		
	# Pick first tick to fit with the spacing and calculate the following tick.
	t0 = math.ceil(lim[0]/d)*d
	n = int((lim[1] - t0)/d) + 1
	return [t0 + d*t for t in range(0,n)]
			
def _calc_limits(datasets,ctxId):
	'''Find the minimum and maximum x and y values in a list of plot lines.
	'''
	xmin = ymin = float('Inf')
	xmax = ymax = float('-Inf')
	for ds in datasets:
		if ds['ctx']==ctxId:
			t = min(ds['xdata'])
			if t < xmin:
				xmin = t
			t = max(ds['xdata'])
			if t > xmax:
				xmax = t
			t = min(ds['ydata'])
			if t < ymin:
				ymin = t
			t = max(ds['ydata'])
			if t > ymax:
				ymax = t		
	return xmin, xmax, ymin, ymax

				
if __name__ == "__main__":  # example and testing
	import ui
	
	# Plot a quadratic and a cubic curve in the same plot.
	xs = [(t-40.0)/30 for t in range(0,101)]
	y1 = [x**2-0.3 for x in xs]
	y2 = [x**3 for x in xs]
	
	CANVAS=0		# draw to canvas or to ui.view
	if CANVAS:
		import canvas_context
		
		# plotting on canvas #
		canvas.set_size(*ui.get_screen_size())
		
		canv_wrld = plottls.rect_area(700,100, 300,300)
		context =canvas_context.canvas_context(canv_wrld)
		grph = xy_graph(context)
	
		grph.set_bounds(40,140, 0,10)
		grph.draw()
		
		context =canvas_context.canvas_context(canv_wrld.offset(-500))
		grph = xy_graph(context)
	
		grph.add_plot(xs, y1, color='red')
		grph.add_plot(xs, y2, color=(0.00, 0.50, 0.80))
		grph.draw()
	else:
		import uiView_context
		# plotting in ui view #
		class gr_view(ui.View):
			""" subclass ui.View to have draw
			"""
			def __init__(self, frame=(100,100,600,300),*args,**kwargs):
				""" clipping all beyond frame
				"""
				super().__init__(frame=frame,*args,**kwargs)
				user = plottls.rect_area(-2,-2,4,6)	# no autoscale
				self.background_color = 'white'
				world = None #plottls.rect_area(50,10,540,260)
				ctx1 = uiView_context.uiView_context(self, world, user)
				self.graph = xy_graph(ctx1)
							
			def draw(self, xar=None, yar=None, col='black'):
				self.graph.add_plot(xar, yar, color=(0.00, 0.00, 1.00))
				self.graph.draw(autoscale=False)
				
		view = gr_view()	
		view.draw(xs,y1,'red')
		view.draw(xs,y2,'blue')
		view.present('sheet')
	
