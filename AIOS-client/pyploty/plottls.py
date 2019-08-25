""" genral purpose tools for drawing to canvas
    including mapping of coordinate systems
"""
from __future__ import division
import canvas

class rect_area(object):
	""" defines a rectangular area for drawing
	"""
	def __init__(self, xmin=0.0,ymin=0.0, xsize=None,ysize=None):
		''' uses canvas size as default
		'''
		self.xmin=xmin
		self.ymin=ymin
		if xsize is None or ysize is None:
			w,h = canvas.get_size()
			if xsize is None:
				xsize=w
			if ysize is None:
				ysize=h
		self.xsize=xsize
		self.ysize=ysize
		
	def __str__(self):
		return 'org:%s size:%s' % (self.get_origin(),self.get_size())
		
	def set_origin(self,xleft,ydown):
		''' left down corner coordinates
		'''
		self.xmin=xleft
		self.ymin=ydown
		
	def set_size(self,width,height):
		self.xsize=width
		self.ysize=height
		
	def get_size(self,xscale=1.0,yscale=None):
		if yscale is None:
			yscale=xscale
		return self.xsize*xscale, self.ysize*yscale
		
	def get_origin(self,xofsFraction=0.0,yofsFraction=None):
		if yofsFraction is None:
			yofsFraction=xofsFraction
		return self.xmin + self.xsize*xofsFraction, self.ymin + self.ysize*yofsFraction
	
	def isInside(self, x,y):
		return len([n for n in (self.xmin, self.xmin+self.xsize) if n<x])==1 and \
			len([n for n in (self.ymin, self.ymin+self.ysize) if n<x])==1
			
	def frame(self):
		""" get tuple of origin(left, low) and size(width,height)
		"""
		return (self.xmin,self.ymin, self.xsize,self.ysize)
		
	def sub_area(self,xofs=0.0,yofs=0.0, xscale=1.0,yscale=1.0):
		return rect_area(*self.get_origin(xofs,yofs), *self.get_size(xscale,yscale))

	def move(self, xoffs=0,yoffs=0):
		self.xmin += xoffs
		self.ymin += yoffs
				
	def offset(self, x=0,y=0):
		return rect_area(x+self.xmin,y+self.ymin  ,*self.get_size())
		
	def yFlipped(self):
		""" returns self but having top bottom (y axis) flipped """
		return rect_area(self.xmin, self.ymin+self.ysize, self.xsize, -self.ysize)

class xy_context(object):
	""" maps 2d user coordinates to world coordinates
	"""
	def __init__(self, world_area=None, user_area=None, font_name='Helvetica', font_size=16.0):
		if user_area is None:
			self.user = rect_area(0.0,0.0, 1.0,1.0)
		elif type(user_area) is tuple:
			self.user = rect_area(*user_area)
		else:
			self.user = user_area		
		if world_area is None:
			w,h = self.get_size()
			self.world = rect_area(0.0,0.0,w,h)
		elif type(world_area) is tuple:
			self.world = rect_area(*world_area)
		else:
			self.world=world_area
		self.font_name=font_name
		self.font_size=font_size
		self.draw_color = (0,0,0,1.0)
		self.emphasis=1
		#print('world:%s user:%s' % (self.world.frame(),self.user.frame()))
	
	def set_size(self,width,height):
		self.user.set_size(width,height)
	def get_size(self):
		return self.user.get_size()
			
	def set_draw_color(self, color):
		self.draw_color=color
			
	def set_emphasis(self, emp=1):
		self.emphasis=emp
		
	def xyWorld(self,xUser=None,yUser=None):
		""" Get world coordinates of org or x,y given
		"""
		if xUser is None:
			xUser=self.user.xmin
		if yUser is None:
			yUser=self.user.ymin
		xt = self.world.xmin + (xUser-self.user.xmin) / (self.user.xsize/self.world.xsize)
		yt = self.world.ymin + (yUser-self.user.ymin) / (self.user.ysize/self.world.ysize)
		return xt,yt
		
	def whWorld(self,wUser=None,hUser=None):
		if wUser is None:
			wt = self.world.xsize
		else:
			wt = wUser / (self.user.xsize/self.world.xsize)
		if hUser is None:
			ht = self.world.ysize
		else:			
			ht = hUser/ (self.user.ysize/self.world.ysize)
		return wt,ht
		
	def xyUser(self):
		return self.user.xmin,self.user.ymin
		
	def whUser(self,wWorld=None,hWorld=None):
		if wWorld is None:
			wt = self.user.xsize
		else:
			wt = wWorld* (self.user.xsize/self.world.xsize)
		if hWorld is None:
			ht = self.user.ysize
		else:
			ht = hWorld* (self.user.ysize/self.world.ysize)
		return wt,ht
		
	def draw_dashed_line(self,x1, y1, x2, y2, ndashes=33):
		'''Draw a dashed line from (x1,y1) to (x2,y2)'''		
		kx = float(x2 - x1) / ndashes
		ky = float(y2 - y1) / ndashes
		self.move_to(x1, y1)
		for i in range(1,ndashes+1):					
			if i % 2 == 1:
				self.add_line(x1 +i*kx, y1 + i*ky)
			else:
				self.move_to(x1 +i*kx, y1 + i*ky)
		self.draw_recorded()
		#self.draw_path()
		
	def record_drawing(self):
		pass
	def draw_recorded(self):
		pass

if __name__ == "__main__":
	import ui
	canvas.set_size(*ui.get_screen_size())
	world = rect_area().sub_area(0.6,0.1, 0.3,0.3)
	user  = rect_area(0,0,100,100)
	cnvs = xy_context(world, user)
	#cnvs.draw_rect(*user.frame())
	subfrm = xy_context(cnvs.world.sub_area(0.1,0.1, 0.8,0.8), cnvs.user)
	#subfrm.draw_rect(*user.frame())
	x,y = cnvs.user.get_origin()
	w,h = cnvs.user.get_size()
	s=w/10
	#cnvs.draw_text('org',0,0)
	#cnvs.draw_text('top',w/2,h)
	#for i in range(int(x+s),int(x+w),int(s)):
	#	subfrm.draw_dashed_line(i,y,i,y+h)
		
