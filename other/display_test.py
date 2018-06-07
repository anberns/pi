# Aaron Berns

from socket import *
import sys
import time
import Adafruit_SSD1306
from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont

# setup lcd, from Adafruit_Python_SSD1306
RST = None
DC = 23
SPI_PORT = 0
SPI_DEVICE = 0
disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)
disp.begin()
#disp.clear()
#disp.display()
width = disp.width
height = disp.height
image = Image.new('1', (width, height))
draw = ImageDraw.Draw(image)
draw.rectangle((0,0,width,height), outline=0, fill=0)
padding = -2
top = padding
botton = height-padding
x=0
font = ImageFont.load_default()

draw.text((x, top), "Howdy there!\nMy name is homey.", font=font, fill=255)
disp.image(image)
disp.display()
time.sleep(1)
disp.clear()
disp.display()

