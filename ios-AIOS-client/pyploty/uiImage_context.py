''' general purpose tool for drawing lines and shapes to pythonista uiImageView class'''

import canvas
import ui
import sys
from PIL import Image
if '.' not in __name__:  # =="__main__" or imported from same package
	sys.path.insert(0,'..')
	import plottls
else:
	from pyploty import plottls
import lib.tls as tls
logger=tls.get_logger(__name__)

IMGDRW = 1  # 1: use uiImageView
TXTDRW = 0	# 1: draw txt directly on view

class uiImage_context(plottls.xy_context):
	""" defines a rectangular drawing frame in a ui.view
	"""
	def __init__(self, uiImageView, *args, **kwargs):
		#world_frame=None, user_frame=None, font_name='Helvetica', font_size=16.0):
		self.uiImageView=uiImageView
		self.ctx = ui.ImageContext(uiImageView.width,uiImageView.height)
		logger.info("context:%s" % self.uiImageView.bounds)
		if IMGDRW:
			#ivfrm =  uiImageView.superview.bounds #uiImageView.frame
			#wrld_frm = (ivfrm[0],ivfrm[1]+ivfrm[3], ivfrm[2],-ivfrm[3]) # chg origin to left under
			wrld_frm =  (0,uiImageView.height, uiImageView.width,-uiImageView.height)
		super().__init__(plottls.rect_area(*wrld_frm), *args, **kwargs) #world_frame,user_frame, font_name, font_size)
	
	def record_drawing(self):
		super().record_drawing()
		
	def draw_recorded(self):
		if IMGDRW:
			w,h = self.uiImageView.width,self.uiImageView.height
			x,y = self.uiImageView.x, self.uiImageView.y #  self.xyWorld()
			with self.ctx:
				super().draw_recorded()
				#ui.Path.stroke()
				img = self.ctx.get_image()
				print('img draw :%.4f,%.4f,%.4f,%.4f size:%s' % (x,y,w,h, img.size))
				#img.show()
				#self.uiImageView.image=img
			self.uiImageView.set_needs_display()
		else:
			super().draw_recorded()
			self.path.stroke()	
	
	def clear(self):
		#self.fill_rect(*self.xyUser(), *self.whUser(), (1,1,1))
		#return
		w,h = self.uiImageView.width,self.uiImageView.height
		x,y = self.uiImageView.x, self.uiImageView.y #  self.xyWorld()
		supvw = self.uiImageView.superview
		supvw.remove_subview(self.uiImageView)
		self.uiImageView = ui.ImageView(frame=(x,y,w,h))
		self.uiImageView.image=None
		supvw.add_subview(self.uiImageView)
		"""
		with ui.ImageContext(w,h) as ctx:
			col=(1,1,1)
			print('clear :%.4f,%.4f,%.4f,%.4f' % (x,y,w,h))
			ui.set_color(col)
			ui.set_color('white')
			ui.set_blend_mode(ui.BLEND_CLEAR)
			ui.fill_rect(x,y,w,h)
			#ui.fill_rect(0,0,600,300)
			img = ctx.get_image()
			self.uiImageView.image = img
			#self.uiImageView.set_needs_display()
			img.draw()
			ui.set_color((0,0,0))
		"""
		#self.draw_recorded()
					
	def draw_rect(self,x,y,width,height):
		with self.ctx:
			rect = ui.Path.rect(*self.xyWorld(x,y), *self.whWorld(width,height))
			rect.stroke()
			return
			img = self.ctx.get_image()
			img.draw()
			#if self.uiImageView.image:
				#self.uiImageView.image= Image.merge('L',(img, self.uiImageView.image))
			#else:
				#self.uiImageView.image=img
				
	def fill_rect(self,xorg,yorg,width,height,color=(1,1,1)):
		w,h =self.whWorld(width,height)
		x,y =self.xyWorld(xorg,yorg)
		print('fill rect xy:%s, wh:%s with:%s' % ((x,y+h), (w,-h), color))
		if IMGDRW:
			with ui.ImageContext(w,-h) as ctx:
				img = Image.new("RGB",(int(w),int(-h)), color)
				#img.draw()
		else:
			ui.set_color(color)
			self.path.append_path(ui.Path.rect())
			ui.fill_rect(x,y, w,-h)
			ui.set_color((0,0,0))
			
					
	def draw_line(self,x1,y1,x2,y2):
		with self.ctx:  # ui.ImageContext(self.uiImageView.width,self.uiImageView.height):  
			#oval = ui.Path.oval(0, 0, 100, 100)
			path = ui.Path()
			path.move_to(*self.xyWorld(x1,y1))
			path.line_to(*self.xyWorld(x2,y2))
			path.stroke()
			return
			img = self.ctx.get_image()
			logger.info("line %s ",((self.xyWorld(x1,y1),self.xyWorld(x2,y2)),img.size))
			img.draw(0,0,300,300)
			path.stroke()
			#return
			if self.uiImageView.image:
				pass
				#self.uiImageView.image= Image.composite(img,self.uiImageView.image,'L')
			else:
				pass
				#self.uiImageView.image=img
			self.uiImageView.set_needs_display()
						
	def draw_text(self,text, x, y, font_name=None, font_size=None):
		if font_size is None:
			font_size=self.font_size
		if font_name is None:
			font_name=self.font_name
		if TXTDRW:
			wl,hl = ui.measure_string(text,0,font=(font_name,font_size))
			xo,yo = self.xyWorld(x,y)  #.whUser(wl,hl)
			frm = (xo,yo,wl,hl)
			print('txtdrw:%s:%s', (text,frm))
			ui.draw_string(text,frm,font=(font_name,font_size))
		else:
			lbl = ui.Label()
			lbl.font = (font_name, font_size)
			lbl.text=text
			lbl.alignment = ui.ALIGN_LEFT
			lbl.size_to_fit()
			wl,hl = lbl.width,lbl.height
			lbl.x,lbl.y= self.xyWorld(x,y)
			lbl.y -= hl 
			if IMGDRW:
				self.uiImageView.add_subview(lbl)
			else:
				self.uiImageView.superview.add_subview(lbl)
		
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

if __name__ == "__main__":
	import time	
	#world =	plottls.drawing_frame(100,100, 600,300)	
	user = plottls.rect_area(0,0, 60,30)
	#ctx = uiView_context(world ,user)
	
	class gr_view(ui.View):
		""" 
		"""
		def __init__(self, frame=(100,100,600,400),*args,**kwargs):
			""" clipping all beyond frame
			"""
			super().__init__(frame=frame,*args,**kwargs)
			#self.bounds = frame # internal coord, origin defaults to 0
			#self.background_color = 'white'
			self.bg_color = 'white'
			ivL = ui.ImageView(frame=(10,10,280,280))
			print('iv.frame=%s iv.bounds=%s self.bounds=%s' % (ivL.frame,ivL.bounds,self.bounds))	
			self.add_subview(ivL)
			ivR = ui.ImageView(frame=(310,10,280,280))
			self.add_subview(ivR)
			self.ctx1 = uiImage_context(ivL, user)
			self.ctx2 = uiImage_context(ivR, user.sub_frame(yofs=-0))
						
		def draw(self):
			self.ctx1.draw_recorded()
			self.ctx2.draw_recorded()
			
	view = gr_view()	
	
	view.ctx1.draw_line(2,1, 30,15)
	view.ctx1.draw_rect(4,2, 52,26)
	view.ctx1.draw_text('org',4,2)
	view.ctx1.draw_text('mid1',30,15)
	view.ctx1.draw_text('top',30,28)
	#view.ctx2.draw_rect(4,2, 52,26)
	view.ctx2.draw_line(2,1, 30,15)
	view.ctx2.draw_text('mid2',30,15)
	#view.ctx2.clear()
	
	view.present('sheet')
	time.sleep(3)
	
	view.draw()
	time.sleep(1)
	#view.ctx1.draw_path()
	print('bye')
