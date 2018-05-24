import os
# read from or create file for this devices settings

addr = "1.2.3.4"
device_settings_path = "./data/" + addr + "_settings"

if os.path.isfile(device_settings_path):
	print "file exists"
	infile = open(device_settings_path, "r")
	items = infile.readline().split()
	temp_low = int(items[0])
	temp_high = int(items[1])
	sound_level = int(items[2])
	motion_level = int(items[3])
	print (temp_low)
	print (motion_level)
	infile.close()

else :
	print "file does not exist"
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


