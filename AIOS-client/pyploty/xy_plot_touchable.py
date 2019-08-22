import canvas
import ui
import operator 

if '.' not in __name__:  # =="__main__" or imported from same package
	import plottls
	import uiView_context
	import xy_plot
else:
	from pyploty import plottls
	from pyploty import uiView_context
	from pyploty import xy_plot

class touch_view(ui.View):
	""" demo showing graphs in a view
	"""
	def __init__(self, frame=(100,100,600,300),*args,**kwargs):
		""" clipping all beyond frame
		"""
		super().__init__(frame=frame,*args,**kwargs)
		self.background_color = 'pink'
		iv = ui.ImageView(frame=(60,10,280,240)) # location within view
		self.add_subview(iv)
		world = plottls.rect_area(*iv.frame)
		user = None # user will autorange to data
		self.context = uiView_context.uiView_context(iv, user)
		self.graph = xy_graph_touchable(self.context)

	def touch_began(self, touch):
		print('touch_began :%s isin:%d world:%s' % (touch.location, self.context.world.isInside(*touch.location),self.context.world.frame()))
		if self.context.world.isInside(*touch.location):
			self.graph.touch_began(touch)
		
	def touch_moved(self, touch):
		#self.graph.touch_moved(touch)
		pass

	def touch_ended(self, touch):
		self.graph.touch_moved(touch)
		pass
		
	def draw(self):
		self.graph.draw(autoscale=True)
		pass

class xy_graph_touchable(xy_plot.xy_graph):
	#def __init__(self, **kwds):
	#	super().__init__(**kwds)
		
	def __init__(self, context, hold=True, grid=True):
		super().__init__(context, hold, grid)
				
	def touch_began(self, touch):
		self.xyStart = touch.location
		self.xyUser = self.context.user.get_origin()
		print('touch began at:%s org=%s' % (touch.location, self.xyUser))
			
	def touch_moved(self, touch):
		moved = touch.location-self.xyStart
		wh = self.context.whUser(*moved)
		#moved= self.context.whUser(*map(sum, zip(xy, self.xyStart)))
		moved= self.context.user.offset(*wh)
		print('touch moved:%s %s' % (moved,self.xyUser))
		#moved = tuple(map(operator.__sub__, self.xyUser, moved))
		self.context.user.move(*moved)   #      #.set_origin(*moved)
		print('new org:%s %s ' % self.context.user.get_origin())
		self.draw(autoscale=False)
		
				
if __name__ == "__main__":
	#import ui
	canvas.set_size(*ui.get_screen_size())
		
	view = touch_view()

	# Plot a quadratic and a cubic curve in the same plot.
	xs = [(t-40.0)/30 for t in range(0,101)]
	y1 = [x**2-0.3 for x in xs]
	y2 = [x**3 for x in xs]

	view.graph.add_plot(xs, y1, color=(0.00, 0.00, 1.00))
	view.graph.add_plot(xs, y2, color=(0.00, 0.50, 0.00))
	#view.graph.draw()
	
	view.present('sheet')
