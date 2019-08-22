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
		''' context= canvas_context or uiView_context object having world set to a drawing area  '''
		self.context = context
		#self.outerbox = world_frame
		self.hold=hold
		self.grid=grid
				
		self.axis_color = (0.40, 0.40, 0.40)
		
		# Initialize non-public attributes
		self.datasets = []
		self.set_bounds(*context.user.frame())
		
	def set_bounds(self, xmin,ymin, xsize, ysize):		
		'''
		Set x- and y-axis user coord limits.
		
		Will also recalculate tick positions.
		'''
		if xsize==0 or ysize==0 or math.isinf(xmin)  or math.isinf(ymin):
			return 
		wrld_frame = self.context.world
		user_frame = plottls.rect_area(xmin,ymin, xsize, ysize)
		logger.info('scaling graph wrld:%s user:%s' % (wrld_frame.frame(),user_frame.frame()))
		#self.innerbox = plottls.canvas_context(wrld_frame, user_frame)
		self.context.user.set_origin(xmin,ymin)
		self.context.user.set_size(xsize, ysize)
		xlim = (xmin, xmin+xsize)
		ylim = (ymin, ymin+ysize)
		self.xticks = _calc_ticks(xlim)
		self.yticks = _calc_ticks(ylim)
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
			_, w, h = self.get_ticklabel(t)
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
		for t in self.yticks:
			_, w, h = self.get_ticklabel(t)
			ws.append(w)
			hs.append(h)
		xmarg[0] = max(max(ws)*1.1, xmarg[0])
		xmarg[1] = max(0, xmarg[1])
		ymarg[0] = max(hs[0]/2 , ymarg[0])
		ymarg[1] = max(hs[-1]/2 , ymarg[1])
		
	def draw_axis(self):
		'''Draw axes and ticks.'''
		#canvas.set_stroke_color(*self.axis_color)
		self.context.set_draw_color(self.axis_color)
		self.context.draw_rect(*self.context.user.frame())
		self.draw_ticks()
		self.rescaling=False
		
	def get_ticklabel(self,tick):
		'''Format a tick label.'''
		lbl = '%.3g' % tick
		w,h = self.context.get_text_size(lbl)
		return lbl, math.fabs(w), math.fabs(h)

	def draw_ticks(self):	
		'''Draw ticks/grid and labels.'''
		w,h = self.context.user.get_size()
		x,y = self.context.user.get_origin()
		for t in self.xticks:			
			if self.grid:
				self.context.draw_dashed_line(t, y, t, y+h)
			else:
				self.context.draw_line(t, y, t, y+h/100)
				self.context.draw_line(t, y+h, t, y+h - h/100)
			lbl, lw, lh = self.get_ticklabel(t)
			self.context.draw_text(lbl, t-lw/2, y-lh)
		
		for t in self.yticks:			
			if self.grid:
				self.context.draw_dashed_line(x, t, x+w, t)
			else:
				self.context.draw_line(x, t, x+w/100, t)
				self.context.draw_line(x+w, t, x+w - w/100, t)
			lbl, lw, lh = self.get_ticklabel(t)
			self.context.draw_text(lbl, x-lw-w*0.01, t-lh/3)

	def add_plot(self, x, y, color=(0.00, 0.00, 0.00), name=None):		
		'''Add plot data.'''	
		plotdata = {'xdata': x, 'ydata': y, 'color': color, 'name':name}
		if self.hold:
			self.datasets.append(plotdata)
		else:
			self.datasets = [plotdata]
		
	def draw(self, autoscale=True):		
		''' draw entire xy_graph as defined by above methods
			canvas specific : override for other target surface			
		'''
		if autoscale:
			xmin,xmax, ymin,ymax = _calc_limits(self.datasets)
			self.set_bounds(xmin,ymin, xmax-xmin,ymax-ymin)
		# All drawing operations are enclose within calls to 'begin_updates and
		# 'end_updates' to improve performance.
		self.context.record_drawing()
		#canvas.begin_updates()
				
		#self.adjust_margins()
		if self.rescaling or not autoscale:
			self.draw_axis()
		
		if not self.datasets:
			# No data to plot but draw axis
			self.context.draw_recorded()
			logger.warning('no chart data')
			return
		
		# Clip plot lines to wihin the axes.
		self.context.draw_rect(*self.context.user.frame())
		#canvas.clip()

		# Plot data as a path with lines between each point.
		
		for ds in self.datasets:
			if ds['ydata']:
				if ds['xdata']:
					xdat=ds['xdata']
				logger.debug('new plot xlen=%d, ylen=%d, col=%s' % (len(xdat),len(ds['ydata']),ds['color']))
				self.context.set_draw_color(ds['color'])
				#canvas.set_stroke_color(*ds['color'])
				self.context.move_to(xdat[0], ds['ydata'][0])
				for i in range(1, len(xdat)):
					self.context.add_line(xdat[i], ds['ydata'][i])
				self.context.draw_recorded()	# color might change
			else:
				logger.warning('empty dataset %s' % ds)

		# Draw all content
		self.context.draw_recorded()
		#canvas.end_updates()
		
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
			
def _calc_limits(datasets):
	'''Find the minimum and maximum x and y values in a list of plot lines.
	'''
	xmin = ymin = float('Inf')
	xmax = ymax = float('-Inf')
	for ds in datasets:
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
	
