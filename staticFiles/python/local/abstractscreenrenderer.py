from PIL import Image
from math import ceil, floor, cos, sin, radians
import os

class abstractscreenrenderer():
	def __init__(self, lookup_path):
		self.width = 0
		self.height = 0
		self.format = "RGB"
		print lookup_path

	def openimage(self, name):
		exts = [ ".png", ".tga", ".dds" ]
		for e in exts:
			path = name + e
			if os.path.isfile(path):
				if e == ".dds":
					return Image.open(path).transpose(Image.FLIP_TOP_BOTTOM)
				return Image.open(path)
		raise FileNotFoundError('image not found: ' + name)
	
	def rotate_and_paste(self, background, foreground, angle, pivot, center_to_pivot, scale_x = 1, scale_y =1):
		rad =  radians(angle)
		# wyznaczamy srodek wokol ktorego zostanie obrocony nakladany obraz	
		rotated = foreground.rotate(angle, expand=1)
		# wyznaczamy pozycje dla nowego srodka 
		rotated_center = ((center_to_pivot[0]*cos(rad)-center_to_pivot[1]*sin(rad))*scale_x,(center_to_pivot[0]*sin(rad)+center_to_pivot[1]*cos(rad))*scale_y)
		# wyznaczamy pozycje srodka na tle (pamietamy o odwroconej osi Y)
		center_postion = (pivot[0] + int(rotated_center[0]), pivot[1] - int(rotated_center[1]))
		# wyznaczamy obszar ktory zajmuje obrocony obrazek 		
		box = (center_postion[0] - int(ceil(rotated.size[0]/2)), center_postion[1] - int(ceil(rotated.size[1]/2)), center_postion[0] + int(floor(rotated.size[0]/2)), center_postion[1] + int(floor(rotated.size[1]/2)))
		background.paste(rotated, box, rotated)
		
	def print_fixed_with(self, draw, text, start_point, character_count, font, color, correction=0):
		rozmiar_osemki = font.getsize("8")[0]
		start_point_tmp = start_point[0]
		for znak in range(len(text), character_count):
			start_point_tmp += rozmiar_osemki-correction
		for znak in range(0,len(text)):
			rozmiar_act = font.getsize(text[znak])[0]
			draw.text((start_point_tmp + rozmiar_osemki - rozmiar_act, start_point[1]), text[znak], font=font, fill=color)
			start_point_tmp += rozmiar_osemki-correction
		
	def print_center(self, draw, text, X, Y, font, color):
		w = draw.textsize(text, font)
		draw.text(((X-w[0]/2),(Y-w[1]/2)), text, font=font, fill=color)

	def print_left(self, draw, text, X, Y, font, color):
		w = draw.textsize(text, font)
		draw.text((X,(Y-w[1]/2)), text, font=font, fill=color)

	def print_right(self, draw, text, X, Y, font, color):
		w = draw.textsize(text, font)
		draw.text(((X-w[0]),(Y-w[1]/2)), text, font=font, fill=color)
		
	def get_width(self):
		return self.__dict__['width']
	
	def get_height(self):
		return self.__dict__['height']

	def get_format(self):
		return self.format if self.format else "RGB"
	
	def render(self, state):
		image = self._render(state)
		self.__dict__['width'] = image.size[0]
		self.__dict__['height'] = image.size[1]
		image = image.transpose(Image.FLIP_TOP_BOTTOM)
		format = self.get_format()
		if image.mode != format:
			image = image.convert(format)
		return image.tobytes("raw", format)

	def getCommands(self):
		commands = self._getCommands()
		return commands

	def _getCommands(self):
		return []

	def manul_set_format(self, format_str):
		self.format = format_str
		return

	def _render(self, state):
		return Image.new("RGB", (1,1), color=(255,105,180))
	

