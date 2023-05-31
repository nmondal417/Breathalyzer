# prints the 2D map data in realtime
#!/usr/bin/env python

import sys
import matplotlib
import timeit

#matplotlib.use("WxAgg")
#matplotlib.use("Agg")
matplotlib.use("TkAgg")
#matplotlib.use("GTKAgg")
#matplotlib.use("Qt4Agg")
#matplotlib.use("CocoaAgg")
#matplotlib.use("MacOSX")

print("***** TESTING WITH BACKEND: %s"%matplotlib.get_backend() + " *****")
print("***** USING MATPLOTLIB VERSION: %s" %matplotlib.__version__ + " *****")

import matplotlib.pyplot as plt
#import matplotlib.animation as animation
import numpy as np
import serial
import time as timer
#import thread

#s = serial.Serial(port='/dev/cu.SLAB_USBtoUART', baudrate=115200)	    # Arduino DUE Programming Port
#s = serial.Serial(port='/dev/tty.usbmodem1411', baudrate=115200)	# Arduino DUE Native Port
s = serial.Serial(port='COM5', baudrate=115200)
f = open("realtime_plotter_neel32_output.txt", "w")

GAIN = -1000
FONTSIZE = 20

VMID = 1.499
VMID_bin = 1883
V_SUPPLY_PCB = 3.2965
V_SUPPLY_ARDUINO = 3.2643
BITS_TO_V = V_SUPPLY_ARDUINO/4095
#BITS_TO_V = 522e-6

IDS_THRESHOLD = 5
def convert_raw_string_to_list(string, nchars):	# divide string by n characters
	ind = 0
	string_list = []
	for i in range(0, len(string), nchars):
		string_list.append(string[i:i+nchars])
		ind = ind + 1
	return string_list

def get_data(time, VDS, VGS, VMID, data, R):	
	string = s.readline()
	f.write(str(string))
	#data = convert_raw_string_to_list(string, 4)
	
	#print string
	if(len(string.decode().split(','))%1028 == 0):
		data = np.fromstring(string, dtype=float, sep=',')		# string to array
		#print(data)
		time = data[0]
		VDS = data[1]
		VGS = data[2]
		VMID = data[3]
		data = data[4:]
		data = data/GAIN
		data = np.reshape(data,(32,32))							# reshape array to 16 x 16
		#data = np.ones((16,16))*100e-6
		VDS_array = np.ones((32,32))*VDS
		#R = np.true_divide(VDS_array,data)
	return [time, VDS, VGS, VMID, data, R]
        		
def run():
	
	VMIN = 0
	VMAX = 350
	time = np.empty(0)
	VDS = np.empty(0)
	VGS = np.empty(0)
	VMID = np.empty(0)
	data = np.zeros((32,32))
	R = np.zeros((32,32))
	data_baseline = np.empty(0)
	time_stamps = np.empty(0)
	VDS_data = np.empty(0)
	VGS_data = np.empty(0)
	data_3d = np.empty(0)
	colorbar_limits = np.empty(0)
	np.set_printoptions(precision=6, suppress=False)
	plt.ion()										# interactive plotting
	plt.show() 										# show plot		
	fig = plt.figure(figsize=(10,10))				# create figure for plot
	im = plt.imshow(data, vmin=VMIN, vmax=VMAX, origin='lower', interpolation='None', animated=True)
	#im = plt.imshow(data, origin='lower',interpolation='none')
	#im = plt.pcolormesh(data)
	plt.pause(0.001)        #fix plt.ion frozen window 

	colorbar = fig.colorbar(im)
	#colorbar.set_cmap('jet')
	#colorbar.set_clim(vmin=VMIN,vmax=VMAX) 
	colorbar.set_label(r'$\mathrm{I_{DS} \ (\mu A)}$', fontsize=FONTSIZE)
	colorbar.draw_all()
	ax = plt.gca()

	for axis in [ax.xaxis, ax.yaxis]:	# Set the major ticks at the centers and minor tick at the edges
		axis.set_ticks(np.arange(32) + 0.5, minor=True)

	ax.grid(b=False, which='major')					# Turn off major and minor grids
	ax.grid(b=True, which='minor')	# Turn on the grid for the minor ticks

	plt.xlabel(r'$\mathrm{Columns}$', fontsize=FONTSIZE)
	plt.ylabel(r'$\mathrm{Rows}$', fontsize=FONTSIZE)
	plt.xticks(np.arange(0, 32, 1), range(32))		# labels every node in the array along the x and y axis
	plt.yticks(np.arange(0, 32, 1), range(32))

	start = 0
	while True:
		
		end = timer.time()
		elapsed = end - start
		fps = 1.0 / elapsed
		start = end
		#print elapsed
		print(fps)		# max python fps is roughly 11 fps

		try:
			time, VDS, VGS, VMID, data, R = get_data(time, VDS, VGS, VMID, data, R)		# get ADC data array
			#VMIN = 1e6*np.amin(data)
			#VMAX = 1e6*np.amax(data)
			VMIN = 0
			VMAX = 350
			#data = 10*np.ones((16,16))		# test dummy data
			IDS_MEAN = np.mean(data)
			print('Average IDS in uA')
			print(IDS_MEAN*1e6)
			print('max current')
			print(np.max(data))
			#print("VDS = ", np.array_str(1e3*VDS), " mV")
			#print "VGS = " + np.array_str(VGS) + " V"
			#print "VOUT = " + np.array_str(IDS_MEAN*GAIN) + " V"
			#print("MEAN IDS = ", np.array_str(1e6*IDS_MEAN), " uA")
			#print "MAX IDS = " + np.array_str(1e6*np.amax(data)) + " uA"
			#print "MIN IDS = " + np.array_str(1e6*np.amin(data)) + " uA"
			#print ("R = ", np.array_str(VDS/IDS_MEAN)," Ohms")
			#print "VDS_bin = " + np.array_str(VDS/BITS_TO_V) + " bits"
			#print "VMID = " + np.array_str(np.mean(VMID)) + " bits"
			#print "Time = " + np.array_str(time) + " ms"

			working_devices = (1e6*data > IDS_THRESHOLD).sum()
			device_yield = 100*np.true_divide(working_devices,1024)
			#print("Yield = ", device_yield," %")
			
			#print "VDS_bin_real = " + np.array_str(VDS/BITS_TO_V+VMID)
			print("\n")

			if(data_3d.size == 0 and np.mean(data) != 0):
				time_stamps = time
				VDS_data = VDS
				VGS_data = VGS
				data_3d = data
				data_baseline = data
				colorbar_limits = np.array([VMIN,VMAX])
			elif(data_baseline.size != 0):
				time_stamps = np.append(time_stamps,time)
				VDS_data = np.append(VDS_data,VDS)
				VGS_data = np.append(VGS_data,VGS)
				data_3d = np.dstack((data_3d,data))
				colorbar_limits = np.dstack((colorbar_limits,np.array([VMIN,VMAX])))

				#print np.amax(data)-np.amin(data)

				im.set_data(1e6*data)	# update data
				#im.set_data(R)	# update data

				#title = "Time = " + np.array_str(time/1000) + " s"
				#plt.title(title)
				#colorbar.set_clim(vmin=VMIN,vmax=VMAX)
				#colorbar.set_label(r'$\mathrm{I_{DS} \ (\mu A)}$', fontsize=FONTSIZE)
				colorbar.draw_all()

				#plt.draw()										# draw data (actually updates figure)
				fig.canvas.draw()								# works as an alternative to plt.draw(), not sure why though		
				fig.canvas.flush_events()
		except KeyboardInterrupt:
			f.close()
			if(os.path.isfile('test.npz')):	# remove previous calibration record (for some reason the save function doesn't simply overwrite)
				os.remove('test.npz')
			np.savez('test', time_stamps=time_stamps, VDS_data=VDS_data, VGS_data=VGS_data, data_3d=data_3d, colorbar_limits=colorbar_limits)
			quit()


if __name__ == '__main__':
	cmd = "\n"
	#s.write(cmd.encode())
	run()

