import logging
import canvas
import ui
from PIL import Image
if '.' not in __name__:  # =="__main__" or imported from same package
	import sys
	sys.path.insert(0,'..')
	#import plottls

from pyploty import plottls
import lib.tls as tls
logger = tls.get_logger(__file__, logging.DEBUG)

#IMGDRW = 0  # 1: use uiImageView
TXTDRW = 1	# 1: draw txt directly on view

class uiView_context(plottls.xy_context):
	""" defines a rectangular drawing frame in a ui.view
	"""
	def __init__(self, inView, world_area=None, *args, **kwargs):
		self.inView=inView
		if not world_area:
			world_area = plottls.rect_area(*inView.bounds).sub_area(0.1,0.05,0.85,0.85)
		elif type(world_area) is tuple:
			world_area = plottls.rect_area(*world_area)
		#elif type(world_area) is scene.Rect:
		#	world_area = plottls.rect_area(*world_area)
		#wrld_frm = (frame[0],frame[1]+frame[3],frame[2],-frame[3]) # have low y at bottom
		self.path=ui.Path()
		super().__init__(world_area.yFlipped(), *args, **kwargs) #world_frame,user_frame, font_name, font_size)
		logger.info('uiView ctx:wrld:%s user:%s' % (self.world.frame(),self.user.frame()))
	
	def record_drawing(self):
		super().record_drawing()
		
	def draw_recorded(self):
		#logger.debug('drawing recorded path:bnds=%s' % self.path.bounds)
		super().draw_recorded()
		self.path.stroke()
		self.path = ui.Path()
	
	def set_draw_color(self,color):
		self.draw_recorded()
		super().set_draw_color(color)
		ui.set_color(color)
		
	def clear(self, uFrame=None):
		#self.fill_rect(*self.xyUser(), *self.whUser(), (1,1,1))
		#return
		if uFrame:
			x,y= self.xyWorld(uFrame[0],uFrame[1])
			w,h= self.whWorld(uFrame[2],uFrame[3])
		else:
			w,h = self.whWorld() 
			x,y = self.xyWorld() 
		self.draw_recorded()	# clears path
		with ui.GState():
			ui.set_color('white')
			path = ui.Path.rect(x,y,w,h)
			#path.eo_fill_rule=False
			#path.close()
			#path.draw()
			path.fill()
					
	def draw_rect(self,x,y,width,height):
		rect = ui.Path.rect(*self.xyWorld(x,y),*self.whWorld(width,height))
		self.path.append_path(rect)
		
	def fill_rect(self,xorg,yorg,width,height,color=(1,1,1)):
		w,h =self.whWorld(width,height)
		x,y =self.xyWorld(xorg,yorg)
		logger.debug('fill rect xy:%s, wh:%s with:%s' % ((x,y+h), (w,-h), color))
		with ui.GState():
			ui.set_color(color)
			ui.set_blend_mode(ui.BLEND_NORMAL)
			rect = ui.Path.rect(x,y+h,w,-h)  # prevent neg height
			rect.eo_fill_rule=True
			rect.close()
			rect.fill()
			self.path.append_path(rect)
			
	def fill_oval(self,xorg,yorg,width,height,color=(1,1,1)):
		w,h =self.whWorld(width,height)
		x,y =self.xyWorld(xorg,yorg)
		with ui.GState():
			ui.set_color(color)
			#ui.set_blend_mode(ui.BLEND_NORMAL)
			oval = ui.Path.oval(x,y+h,w,-h)  # prevent neg height
			#rect.eo_fill_rule=True
			#rect.close()
			oval.fill()
			self.path.append_path(oval)
				
	def draw_line(self,x1,y1,x2,y2):
		self.move_to(x1,y1)
		self.path.line_to(*self.xyWorld(x2,y2))
				
	def draw_text(self,text, x, y, font_name=None, font_size=None):
		if font_size is None:
			font_size=self.font_size
		if font_name is None:
			font_name=self.font_name
		if TXTDRW:
			wl,hl = ui.measure_string(text,0,font=(font_name,font_size))
			xo,yo = self.xyWorld(x,y)  #.whUser(wl,hl)
			frm = (xo,yo-hl,wl,hl)
			#logger.debug('txtdrw:%s:%s fnt:%s,%s' % (text,(frm),font_name,font_size) )
			ui.draw_string(text,frm,font=(font_name,font_size), color='black', alignment=ui.ALIGN_NATURAL, line_break_mode=ui.LB_WORD_WRAP)			
		else:
			lbl = ui.Label()
			lbl.font = (font_name, font_size)
			lbl.text=text
			lbl.alignment = ui.ALIGN_LEFT
			lbl.size_to_fit()
			wl,hl = lbl.width,lbl.height
			lbl.x,lbl.y= self.xyWorld(x,y)
			lbl.y -= hl 
			self.inView.add_subview(lbl)
		
	def move_to(self,x,y):
		self.path.move_to(*self.xyWorld(x,y))
		
	def add_line(self,x,y):
		self.path.line_to(*self.xyWorld(x,y))
		
	def add_rect(self,x,y,width,height):
		rect = ui.Path.rect(*self.xyWorld(x,y),*self.whWorld(width,height))
		self.path.append_path(rect)
					
	def add_curve(cp1x, cp1y, cp2x, cp2y, x, y):
		self.path.add_curve(*self.xyWorld(cp1x,cp1y), *self.xyWorld(cp2x,cp2y), *self.xyWorld(x,y))
	
	def get_text_size(self, text, font_name=None, font_size=None):
		''' size of text string in user coordinates '''		
		if font_size is None:
			font_size=self.font_size
		if font_name is None:
			font_name=self.font_name
		return self.whUser(*canvas.get_text_size(text, font_name, font_size))	

class vwDigitals(ui.View):
	bitvals=[]
	modes=[]
	def draw(self):
		if self.bitvals:
			n = len(self.bitvals)
			logger.debug('dig bnds:%s' % self.bounds)
			world = plottls.rect_area(*self.bounds)
			ctx = uiView_context(self, world_area=world, user_area=(0,0,n,1) ) # plottls.rect_area(0,0,n,1))
			#logger.debug('digitals:%s world:%s' % (self.bitvals, ctx.world.frame()))
			for i,bitv,mode in zip(range(n), self.bitvals, self.modes):
				if mode == 0b11:
					ctx.fill_oval(i+0.05,0.05,0.9,0.9, 'grey')
				else:
					ctx.fill_oval(i+0.05,0.05,0.9,0.9, 'brown' if mode==0 else 'beige')
					ctx.fill_oval(i+0.2,0.2,0.6,0.6, (0.922,0.835,0.652,1) if bitv else 'black')

if __name__ == "__main__":	# testing and examples
	import time	
	user = plottls.rect_area(0,0, 60,30)

	class gr_view(ui.View):
		""" 
		"""
		def __init__(self, frame=(100,100,600,400),*args,**kwargs):
			""" (not) clipping all beyond frame
			"""
			super().__init__(frame=frame,*args,**kwargs)
			self.add_subview(vwDigitals(frame=(10,340,580,30),background_color='green',name='digitals'))
			self.bg_color = 'white'
			frL=plottls.rect_area(10,10,280,280)
			frR=frL.offset(300)
			print('iv.frame=%s iv.bounds=%s ' % (frL.frame(),self.bounds))	
			self.ctx1 = uiView_context(self, world_area=frL, user_area=user)
			self.ctx2 = uiView_context(self, world_area=frR, user_area=user.sub_area(yofs=1, yscale=-1))
						
		def draw(self):
			for i,ctx,col in zip(range(2), (self.ctx1,self.ctx2), ('orange',(0.30,1,1,0.5))):
				ctx.draw_line(2,1, 30,15)
				ctx.add_rect(4,2, 52,26)
				ctx.draw_text('org%d' % i,4,2)
				ctx.fill_rect(24,13,12,4,col)
				ctx.draw_text('midden%d' % i,30,15)  # is overwriiten by filled rect !!
				ctx.draw_text('top%d' % i,30,28)
				ctx.draw_text('bottom%d' % i, 4,-4)
				ctx.draw_recorded()
				
	view = gr_view()	
	
	view.present('sheet')
	view['digitals'].bitvals= [0,0,1,1,1,0,0,0,1,1,1,0]
	view['digitals'].modes=   [0,0,0,1,1,1,1,1,1,0,0,0]
	view['digitals'].set_needs_display()
	time.sleep(3)
	print('bye')
