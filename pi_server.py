# Aaron Berns

import os
import thread
import threading
from socket import *
import sys
import time
import Adafruit_SSD1306
from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont
import RPi.GPIO as GPIO

# thread locks
try:
	led_lock = threading.Lock() # controls access to led screen
	event_lock = threading.Lock() # controls access to buzzer
	global_var_lock = threading.Lock()
except IOError:
	print("Unable to create thread locks")
	exit(1)

# setup lcd, from Adafruit_Python_SSD1306
RST = None
DC = 23
SPI_PORT = 0
SPI_DEVICE = 0
disp = Adafruit_SSD1306.SSD1306_128_64(rst=RST)

try:
	disp.begin()
except IOError:
	print("Unable to connect to display")
	exit(1)
else:
	disp.clear()
	disp.display()
	width = disp.width
	height = disp.height
	image = Image.new('1', (width, height))
	draw = ImageDraw.Draw(image)
	draw.rectangle((0,0,width,height), outline=0, fill=0)

# display zones
top = 0 
ztop0 = top 		#ip address or name of client
zbottom0 = ztop0+8
ztop1 = zbottom0+8  #temp
zbottom1 = ztop1+8
ztop2 = zbottom1+2  #sound alert
zbottom2 = ztop2+8
ztop3 = zbottom2+2  #motion alert
zbottom3 = ztop3+8
ztop4 = zbottom3+2  #smoke alert
zbottom4 = ztop4+8
bottom = height 
x=0

# display options
font = ImageFont.load_default()

# sockets
try:
	serverSocket = socket(AF_INET, SOCK_STREAM)
	serverSocket.bind(('', int(sys.argv[1]))) 
	serverSocket.listen(1)
except IOError:
	print("Unable to create listening socket")
	exit(1)

# global emergency / alert flags
global smoke_event

global current_temp 	#global temperature variable
global sound_freq 		#global sound event 'counter'
global motion_freq		#global motion event 'counter'

# global off button pressed variable
global end

# global settings variables
global temp_low 
global temp_high
global sound_level
global motion_level

# buzzer setup
buzzer = 18
try:
	GPIO.setup(buzzer, GPIO.OUT)
except IOError:
	print("Unable to setup buzzer")
	exit(1)
else:
	GPIO.output(buzzer, True)

# button setup
off_button = 19
left_button = 13
up_button = 6
down_button = 21
right_button = 5

try:
	GPIO.setup(off_button, GPIO.IN, pull_up_down=GPIO.PUD_UP)
	GPIO.setup(left_button, GPIO.IN, pull_up_down=GPIO.PUD_UP)
	GPIO.setup(up_button, GPIO.IN, pull_up_down=GPIO.PUD_UP)
	GPIO.setup(down_button, GPIO.IN, pull_up_down=GPIO.PUD_UP)
	GPIO.setup(right_button, GPIO.IN, pull_up_down=GPIO.PUD_UP)
except IOError:
	print("Unable to setup buttons")
	exit(1)

# called when temp message received from client
def updateTemp(temp):
	global current_temp

	global_var_lock.acquire()
	current_temp = temp 
	global_var_lock.release()

	led_lock.acquire()
	draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
	draw.text((x, ztop1), current_temp, font=font, fill=255)
	disp.image(image)
	disp.display()
	led_lock.release()

# checks that temp is within set range, alerts if not
def monitorTemp():
	global current_temp
	global temp_low 
	global temp_high 

	time.sleep(10)
	while 1:
		global_var_lock.acquire()
		local_temp = current_temp
		float_temp = float(local_temp[5:9])
		global_var_lock.release()

		# buzz and display alert if temp out of range
		if float_temp > float(temp_high) or float_temp < float(temp_low):
			event_lock.acquire()
			GPIO.output(buzzer, False)
			time.sleep(.01)
			GPIO.output(buzzer, True)
			time.sleep(.01)
			event_lock.release()
			led_lock.acquire()
			draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
			draw.text((x, ztop1), "TEMP ALERT", font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
			time.sleep(2)
			led_lock.acquire()
			draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
			draw.text((x, ztop1), local_temp, font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
		time.sleep(4) #save cpu usage during while loop

# responds to sound messages based on sensitivity settings
def soundEvent():
	global sound_freq
	global sound_level

	global_var_lock.acquire()
	local_level = sound_level
	global_var_lock.release()

	# get seconds elapsed from last event
	elasped = int(time.time()) - sound_freq
	sound_freq = int(time.time())

	# check settings for sensitivity, alert if needed
	if local_level == 2:  #any sound message causes alert
			led_lock.acquire()
			draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)
			draw.text((x, ztop2), "SOUND ALERT", font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
			event_lock.acquire()
			GPIO.output(buzzer, False)
			time.sleep(.01)
			GPIO.output(buzzer, True)
			time.sleep(.01)
			event_lock.release()
			time.sleep(2)
			led_lock.acquire()
			draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)		
			disp.image(image)
			disp.display()
			led_lock.release()

	elif local_level == 1 and elasped <= 2:  #sustained sound causes alert
			led_lock.acquire()
			draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)
			draw.text((x, ztop2), "SOUND ALERT", font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
			event_lock.acquire()
			GPIO.output(buzzer, False)
			time.sleep(.01)
			GPIO.output(buzzer, True)
			time.sleep(.01)
			event_lock.release()
			time.sleep(2)
			led_lock.acquire()
			draw.rectangle((0,ztop2,width,zbottom2), outline=0, fill=0)		
			disp.image(image)
			disp.display()
			led_lock.release()

	# setting of 0 ignores sensor messages

# responds to smoke messages, alerts until smoke dissipates
def smokeEvent():
	global smoke_event

	#first smoke message sounds alarm until second received
	led_lock.acquire()
	draw.rectangle((0,ztop4,width,zbottom4), outline=0, fill=0)
	draw.text((x, ztop4), "SMOKE EMERGENCY", font=font, fill=255)
	disp.image(image)
	disp.display()
	led_lock.release()

	# get initial status of smoke alarm
	event_lock.acquire()
	local_smoke = smoke_event
	event_lock.release()

	# buzz until smoke clears
	while local_smoke:
		event_lock.acquire()
		GPIO.output(buzzer, False)
		time.sleep(1)
		GPIO.output(buzzer, True)
		time.sleep(1)
		local_smoke = smoke_event
		event_lock.release()

	# clear alert
	led_lock.acquire()
	draw.rectangle((0,ztop4,width,zbottom4), outline=0, fill=0)
	disp.image(image)
	disp.display()
	led_lock.release()

# responds to motion messages based on sensitivity settings
def motionEvent():
	global motion_freq
	global motion_level

	global_var_lock.acquire()
	local_level = motion_level
	global_var_lock.release()

	# get seconds elapsed from last event
	elasped = int(time.time()) - motion_freq
	motion_freq = int(time.time())

	# check settings for sensitivity, alert if needed

	if local_level == 2:  #any message creates alert
			led_lock.acquire()
			draw.rectangle((0,ztop3,width,zbottom3), outline=0, fill=0)
			draw.text((x, ztop3), "MOTION ALERT", font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
			event_lock.acquire()
			GPIO.output(buzzer, False)
			time.sleep(.01)
			GPIO.output(buzzer, True)
			time.sleep(.01)
			event_lock.release()
			time.sleep(2)
			led_lock.acquire()
			draw.rectangle((0,ztop3,width,zbottom3), outline=0, fill=0)		
			disp.image(image)
			disp.display()
			led_lock.release()

	elif local_level == 1 and elasped <= 2:  #sustained motion creates alert
			led_lock.acquire()
			draw.rectangle((0,ztop3,width,zbottom3), outline=0, fill=0)
			draw.text((x, ztop3), "MOTION ALERT", font=font, fill=255)
			disp.image(image)
			disp.display()
			led_lock.release()
			event_lock.acquire()
			GPIO.output(buzzer, False)
			time.sleep(.01)
			GPIO.output(buzzer, True)
			time.sleep(.01)
			event_lock.release()
			led_lock.acquire()
			time.sleep(2)
			draw.rectangle((0,ztop3,width,zbottom3), outline=0, fill=0)		
			disp.image(image)
			disp.display()
			led_lock.release()

	# setting of 0 ignores messages

# runs the settings menu process
def runMenu():

	while 1:

		#thread waits for initial right button press
		# help with polling from sourceforge.net
		GPIO.wait_for_edge(right_button, GPIO.RISING)
		pointer_location = 0 	#menu pointer begins on low temp
		global current_temp
		led_lock.acquire()
			
		displaySettings(pointer_location)

		# left button from main menu exits settings
		while GPIO.input(left_button) != 1:
			time.sleep(.1)  #conserve cpu by blocking loop

			#move pointer on display based on up and down presses
			if GPIO.input(up_button):
				time.sleep(.05)  #prevent multiple movements per short press
				if pointer_location > 0:
					pointer_location = pointer_location -1
					displaySettings(pointer_location)
			elif GPIO.input(down_button):
				time.sleep(.05)
				if pointer_location < 3:
					pointer_location = pointer_location +1
					displaySettings(pointer_location)

			#another right press allows for setting adjusment based on pointer position
			elif GPIO.input(right_button):
				time.sleep(.05)
				if pointer_location == 0:
					adjustHighTemp()
				elif pointer_location == 1:
					adjustLowTemp()
				elif pointer_location == 2:
					adjustSoundLevel()
				elif pointer_location == 3:
					adjustMotionLevel()
				displaySettings(pointer_location)

		#redraw default dispay
		draw.rectangle((0,0,width,height), outline=0, fill=0)
		draw.text((x, ztop0), addr[0], font=font, fill=255)
		global_var_lock.acquire()
		draw.text((x, ztop1), current_temp, font=font, fill=255)
		global_var_lock.release()
		disp.image(image)
		disp.display()
		led_lock.release()

# allows temp at low range to be adjusted using up and down buttons
def adjustLowTemp():
	global temp_low 

	draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
	draw.text((x, ztop0), "Low temperature", font=font, fill=255)
	draw.text((x, ztop1), str(temp_low), font=font, fill=255)
	disp.image(image)
	disp.display()

	#left button returns to main menu
	while GPIO.input(left_button) != 1:
		time.sleep(.1) #conserve cpu

		#adjust temp
		if GPIO.input(up_button):
			time.sleep(.05)
			if temp_low < 125:
				temp_low = temp_low + 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(temp_low), font=font, fill=255)
				disp.image(image)
				disp.display()
		elif GPIO.input(down_button):
			time.sleep(.05)
			if temp_low > -25:
				temp_low = temp_low - 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(temp_low), font=font, fill=255)
				disp.image(image)
				disp.display()

# allows temp at high range to be adjusted using up and down buttons
# can be refactored
def adjustHighTemp():
	global temp_high 

	draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
	draw.text((x, ztop0), "High temperature", font=font, fill=255)
	draw.text((x, ztop1), str(temp_high), font=font, fill=255)
	disp.image(image)
	disp.display()

	while GPIO.input(left_button) != 1:
		time.sleep(.1)
		if GPIO.input(up_button):
			time.sleep(.05)
			if temp_high < 150:
				temp_high = temp_high + 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(temp_high), font=font, fill=255)
				disp.image(image)
				disp.display()
		elif GPIO.input(down_button):
			time.sleep(.05)
			if temp_high > -10:
				temp_high = temp_high - 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(temp_high), font=font, fill=255)
				disp.image(image)
				disp.display()

# allows sound sensitivity to be adjusted
def adjustSoundLevel():
	global sound_level 

	draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
	draw.text((x, ztop0), "Sound level", font=font, fill=255)
	draw.text((x, ztop1), str(sound_level), font=font, fill=255)
	disp.image(image)
	disp.display()

	#left button exits to main menu
	while GPIO.input(left_button) != 1:
		time.sleep(.1)

		if GPIO.input(up_button):
			time.sleep(.05)
			if sound_level < 2:
				sound_level = sound_level + 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(sound_level), font=font, fill=255)
				disp.image(image)
				disp.display()
		elif GPIO.input(down_button):
			time.sleep(.05)
			if sound_level > 0:
				sound_level = sound_level - 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(sound_level), font=font, fill=255)
				disp.image(image)
				disp.display()

# allows motion sensitivity to be adjusted
# can be refactored with sound
def adjustMotionLevel():
	global motion_level 

	draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
	draw.text((x, ztop0), "Motion level", font=font, fill=255)
	draw.text((x, ztop1), str(motion_level), font=font, fill=255)
	disp.image(image)
	disp.display()

	while GPIO.input(left_button) != 1:
		time.sleep(.1)

		if GPIO.input(up_button):
			time.sleep(.05)
			if motion_level < 2:
				motion_level = motion_level + 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(motion_level), font=font, fill=255)
				disp.image(image)
				disp.display()
		elif GPIO.input(down_button):
			time.sleep(.05)
			if motion_level > 0:
				motion_level = motion_level - 1
				draw.rectangle((0,ztop1,width,zbottom1), outline=0, fill=0)
				draw.text((x, ztop1), str(motion_level), font=font, fill=255)
				disp.image(image)
				disp.display()

# displays main menu on lcd based on pointer location movement
def displaySettings(pointer_location):
	global temp_low 
	global temp_high 
	global sound_level 
	global motion_level 

	if pointer_location == 0:
		draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
		draw.text((x, ztop0), "Adjust settings", font=font, fill=255)
		draw.text((x, ztop1), "->High temp: " + str(temp_high), font=font, fill=255)
		draw.text((x, ztop2), "Low temperature: " + str(temp_low), font=font, fill=255)
		draw.text((x, ztop3), "Sound sensitivity: " + str(sound_level), font=font, fill=255)
		draw.text((x, ztop4), "Motion sensitivity: " + str(motion_level), font=font, fill=255)
		disp.image(image)
		disp.display()
	elif pointer_location == 1:
		draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
		draw.text((x, ztop0), "Adjust settings", font=font, fill=255)
		draw.text((x, ztop1), "High temperature: " + str(temp_high), font=font, fill=255)
		draw.text((x, ztop2), "->Low temp: " + str(temp_low), font=font, fill=255)
		draw.text((x, ztop3), "Sound sensitivity: " + str(sound_level), font=font, fill=255)
		draw.text((x, ztop4), "Motion sensitivity: " + str(motion_level), font=font, fill=255)
		disp.image(image)
		disp.display()
	elif pointer_location == 2:
		draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
		draw.text((x, ztop0), "Adjust settings", font=font, fill=255)
		draw.text((x, ztop1), "High temperature: " + str(temp_high), font=font, fill=255)
		draw.text((x, ztop2), "Low temperature: " + str(temp_low), font=font, fill=255)
		draw.text((x, ztop3), "->Sound sens: " + str(sound_level), font=font, fill=255)
		draw.text((x, ztop4), "Motion sensitivity: " + str(motion_level), font=font, fill=255)
		disp.image(image)
		disp.display()
	elif pointer_location == 3:
		draw.rectangle((0,ztop0,width,zbottom4), outline=0, fill=0)
		draw.text((x, ztop0), "Adjust settings", font=font, fill=255)
		draw.text((x, ztop1), "High temperature: " + str(temp_high), font=font, fill=255)
		draw.text((x, ztop2), "Low temperature: " + str(temp_low), font=font, fill=255)
		draw.text((x, ztop3), "Sound sensitivity: " + str(sound_level), font=font, fill=255)
		draw.text((x, ztop4), "->Motion sens: " + str(motion_level), font=font, fill=255)
		disp.image(image)
		disp.display()

global_var_lock.acquire()
smoke_event = 0
end = 0
global_var_lock.release()

#wait for connection from client
connectionSocket, addr = serverSocket.accept()

# read from or create file for this device's settings
device_settings_path = "./data/" + addr[0] + "_settings"

try:
	if os.path.isfile(device_settings_path):
		infile = open(device_settings_path, "r")
		items = infile.readline().split()
		temp_low = int(items[0])
		temp_high = int(items[1])
		sound_level = int(items[2])
		motion_level = int(items[3])
		infile.close()

	else : #store defaults in new file
		temp_low = 50
		temp_high = 90
		sound_level = 2
		motion_level = 2
		outfile = open(device_settings_path, "w+")
		outfile.write(str(temp_low) + " ")
		outfile.write(str(temp_high) + " ")
		outfile.write(str(sound_level) + " ")
		outfile.write(str(motion_level) + " ")
		outfile.close()
except IOError:
	print("Unable to access device settings file, using defaults")
	temp_low = 50
	temp_high = 90
	sound_level = 2
	motion_level = 2

# start display and sensor reporting
draw.text((x, ztop0), "Hello", font=font, fill=255)
disp.image(image)
disp.display()
time.sleep(2)
draw.rectangle((0,0,width,height), outline=0, fill=0)
draw.text((x, ztop0), addr[0], font=font, fill=255)
disp.image(image)
disp.display()

# launch settings menu thread
try:
	thread.start_new_thread(runMenu, ())
except IOError:
	print("Menu thread failed")

# launch temp monitoring thread
try:
	thread.start_new_thread(monitorTemp, ())
except IOError:
	print("Temperature monitoring thread failed")

# set current time for global event message counters
sound_freq = int(time.time())
motion_freq = int(time.time())

#set end to 0
global_var_lock.acquire()
end = 0
global_var_lock.release()

#sensor monitoring, new thread created when message received 
while(1):

	#wait for message from client
	message = connectionSocket.recv(1024).decode()

	# sensor concurrency
	if 'temp' in message:
		try:
			thread.start_new_thread(updateTemp, (message,))
		except IOError:
			print("Temperature update thread failed")

	elif message == "smoke":

		event_lock.acquire()

		if smoke_event == 0:
			smoke_event = 1
			event_lock.release()

			#if new thread fails, take over main thread
			try:
				thread.start_new_thread(smokeEvent, ())
			except IOError:
				print("Thread spawn failed")
				smokeEvent()
		else:
			smoke_event = 0
			event_lock.release()

	elif message == "sound":
		try:
			thread.start_new_thread(soundEvent, ())
		except IOError:
			print("Thread spawn failed")

	elif message == "motion":
		try:
			thread.start_new_thread(motionEvent, ())
		except IOError:
			print("Thread spawn failed")

	elif message == "disconnect":
		break
	

# save updated settings
try:
	outfile = open(device_settings_path, "w")
except IOError:
	print("Unable to save settings")
else:
	outfile.write(str(temp_low) + " ")
	outfile.write(str(temp_high) + " ")
	outfile.write(str(sound_level) + " ")
	outfile.write(str(motion_level) + " ")
	outfile.close()

# close connection 
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
