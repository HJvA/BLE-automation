from __future__ import division
import canvas,ImageColor

if '.' not in __name__:  # =="__main__" or imported from same package
	import plottls
else:
	from pyploty import plottls

	
class canvas_context(plottls.xy_context):
	""" maps user coordinates to world (ios pixel canvas) coordinates
	"""		
	def record_drawing(self):
		canvas.begin_updates()
	
	def draw_recorded(self):
		canvas.draw_path()
		canvas.end_updates()
	
	def set_draw_color(self,color):
		super().set_draw_color(color)
		if type(color) is tuple:
			canvas.set_stroke_color(*color)
		else:
			canvas.set_stroke_color(*ImageColor.getrgb(color))
		
	def draw_rect(self,x,y,width,height):
		return canvas.draw_rect(*self.xyWorld(x,y),*self.whWorld(width,height))
		
	def draw_line(self,x1,y1,x2,y2):
		return canvas.draw_line(*self.xyWorld(x1,y1),*self.xyWorld(x2,y2))
				
	def draw_text(self,text, x, y, font_name=None, font_size=None):
		if font_size is None:
			font_size=self.font_size
		if font_name is None:
			font_name=self.font_name
		canvas.draw_text(text, *self.xyWorld(x,y), font_name, font_size)
		
	def move_to(self,x,y):
		canvas.move_to(*self.xyWorld(x,y))
		
	def add_line(self,x,y):
		canvas.add_line(*self.xyWorld(x,y))
		
	def add_rect(self,x,y,width,height):
		canvas.add_rect(*self.xyWorld(x,y), *self.whWorld(width,height))
		
	def clear(self):
		x,y = self.xyWorld(*self.xyUser())
		w,h = self.whWorld(*self.whUser())
		canvas.set_fill_color(1,1,1)
		canvas.fill_rect(x,y,w,h)
		#canvas.set_fill_color(self.draw_color)
	
	def fill_rect(self,x,y,width,height,color=(1,1,1)):
		#canvas.set_blend_mode(canvas.BLEND_CLEAR)
		canvas.set_fill_color(color)
		canvas.fill_rect(*self.xyWorld(x,y), *self.whWorld(width,height))
			
	def add_curve(cp1x, cp1y, cp2x, cp2y, x, y):
		canvas.add_curve(*self.xyWorld(cp1x,cp1y), *self.xyWorld(cp2x,cp2y), *self.xyWorld(x,y))
	
	def get_text_size(self, text, font_name=None, font_size=None):
		''' size of text string in user coordinates '''		
		if font_size is None:
			font_size=self.font_size
		if font_name is None:
			font_name=self.font_name
		return self.whUser(*canvas.get_text_size(text, font_name, font_size))		

if __name__ == "__main__":
	from time import sleep
	canvas.set_size(1000,700)	
	world =	plottls.rect_area(100,100, 300,300)	
	user = plottls.rect_area(0,0, 60,30)
	#ctx = uiView_context(world ,user)
			
	ctx1 = canvas_context(world, user)
	ctx2 = canvas_context(world, user.sub_area(xofs=-1.2))			# left of world frame
	
	ctx1.draw_line(2,1, 30,15)
	ctx1.add_rect(4,2, 52,26)
	ctx1.draw_text('org',4,2)
	ctx1.draw_text('mid1',30,15)
	ctx1.draw_text('top',30,28)
	ctx2.add_rect(4,2, 52,26)
	ctx2.draw_text('mid2',30,15)
	
	ctx1.draw_recorded()
	ctx2.draw_recorded()
	sleep(3)
	ctx2.clear()
