# Aaron Berns

import thread
import threading
from socket import *
import sys
import time
import Adafruit_SSD1306
from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont

# thread lock
lock = threading.Lock()

# setup lcd, from Adafruit_Python_SSD1306
RST = None
DC = 23
SPI_PORT = 0
SPI_DEVICE = 0
disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)
disp.begin()
disp.clear()
disp.display()
width = disp.width
height = disp.height
image = Image.new('1', (width, height))
draw = ImageDraw.Draw(image)
draw.rectangle((0,0,width,height), outline=0, fill=0)

# display zones
top = 0 
ztop0 = top
zbottom0 = ztop0+8
ztop1 = zbottom0+8 
zbottom1 = ztop1+8
ztop2 = zbottom1+2 
zbottom2 = ztop2+8
bottom = height 
x=0

# display options
font = ImageFont.load_default()

# sockets
serverSocket = socket(AF_INET, SOCK_STREAM)
serverSocket.bind(('', int(sys.argv[1]))) 
serverSocket.listen(1)

def soundEvent():
	print ("sound")

	lock.acquire()
	draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
	draw.text((x, ztop1), "sound", font=font, fill=255)
	disp.image(image)
	disp.display()
	lock.release()
	time.sleep(2)
	lock.acquire()
	draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
	disp.image(image)
	disp.display()
	lock.release()

	connectionSocket.send(("rec").encode())

def motionEvent():
	print ("motion")

	lock.acquire()
	draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)
	draw.text((x, ztop2), "motion", font=font, fill=255)
	disp.image(image)
	disp.display()
	lock.release()
	time.sleep(2)
	lock.acquire()
	draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)		
	disp.image(image)
	disp.display()
	lock.release()

	connectionSocket.send(("rec").encode())

# loop through connections from listening port until CTRL-C
while 1:
	
	connectionSocket, addr = serverSocket.accept()
	print ("Connection from " + addr[0])
	draw.text((x, ztop0), "Hello", font=font, fill=255)
	disp.image(image)
	disp.display()
	time.sleep(2)
	draw.rectangle((0,0,width,height), outline=0, fill=0)
	draw.text((x, ztop0), addr[0], font=font, fill=255)
	disp.image(image)
	disp.display()

	while(1):
		message = connectionSocket.recv(1024).decode()
	
		# sensor concurrency
		if message == "sound":
			thread.start_new_thread(soundEvent, ())

		elif message == "motion":
			thread.start_new_thread(motionEvent, ())

		elif message == "disconnect":
			break
		
		else:
			connectionSocket.send(("error").encode())
		

	# close connection but not server process
	print ("Closing connection from " + addr[0])
	disp.clear()
	disp.display()
	draw.rectangle((0,0,width,height), outline=0, fill=0)
	draw.text((x, top), "Goodbye", font=font, fill=255)
	disp.image(image)
	disp.display()
	time.sleep(2)
	disp.clear()
	disp.display()
	connectionSocket.close()


