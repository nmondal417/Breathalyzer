#!/usr/bin/env python

import sys
import matplotlib
import timeit
import os

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
import matplotlib.animation as animation
import numpy as np
import serial
import time as timer
#import thread

#s = serial.Serial(port='/dev/cu.SLAB_USBtoUART', baudrate=115200)	# Arduino DUE Programming Port
#s = serial.Serial(port='/dev/tty.usbmodem1411', baudrate=115200)	# Arduino DUE Native Port
s = serial.Serial(port='COM6', baudrate=115200)

GAIN = -1000
FONTSIZE = 20           # fontsize for plotss
LINEWIDTH = 1.5
XLABELPAD = 5           # pad the xlabel away from the xtick labels
YLABELPAD = 10          # pad the ylabel away from the ytick labels

VMID = 1.499
VMID_bin = 1883
V_SUPPLY_PCB = 3.2965
V_SUPPLY_ARDUINO = 3.2662
BITS_TO_V = V_SUPPLY_ARDUINO/4095



YIELD_THRESHOLD_low = 1
YIELD_THRESHOLD_high = 400

def convert_raw_string_to_list(string, nchars):	# divide string by n characters
	ind = 0
	string_list = []
	for i in range(0, len(string), nchars):
		string_list.append(string[i:i+nchars])
		ind = ind + 1
	return string_list

def get_data(time, VDS, VGS, VMID, data, R):	
	string = s.readline()
	#print string
	#data = convert_raw_string_to_list(string, 4)
	#print string
	if(len(string.decode().split(','))%1028 == 0):
		data = np.fromstring(string, dtype=float, sep=',')		# string to array
		#print data
		time = data[0]
		VDS = data[1]
		VGS = data[2]
		VMID = data[3]
		#data = data[4:]*BITS_TO_V
		data = data[4:]
		data = data/GAIN
		data = np.reshape(data,(32,32))							# reshape array to 16 x 16
		VDS_array = np.ones((32,32))*VDS
		#R = np.true_divide(VDS_array,data)
		R = 0
	return [time, VDS, VGS, VMID, data, R]
        		
def run():

	print ("\n\n1. Enter Descriptive Data File Name: "),
	data_file_name = input()

	VMIN = 0
	VMAX = 100
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

	plt.ion()										# interactive plotting
	#plt.show() 										# show plot		
	#fig = plt.figure(figsize=(10,10))				# create figure for plot
	fig = plt.figure()				# create figure for plot
	plt.grid(True)
	ax = plt.gca()
	plt.xlabel(r'$\mathrm{Time\ (s)}$', fontsize=FONTSIZE, labelpad=XLABELPAD)
	plt.ylabel(r'$\mathrm{\Delta I_{DS}/\overline{I_{DS}}\ (\%)}$', labelpad=YLABELPAD)
    
	start = 0
	while True:
		
		end = timer.time()
		elapsed = end - start
		fps = 1.0 / elapsed
		start = end
		#print elapsed
		#print fps			# max python fps is roughly 11 fps

		try:

			time, VDS, VGS, VMID, data, R = get_data(time, VDS, VGS, VMID, data, R)		# get ADC data array
			#print "got data"
			#VMIN = 1e6*np.amin(data)
			#VMAX = 1e6*np.amax(data)
			VMIN = 0
			VMAX = 160
			#IDS_MEAN = np.mean(data)
			#print "VDS = " + np.array_str(1e3*VDS) + " mV"
			#print "VGS = " + np.array_str(VGS) + " V"
			#print "VOUT = " + np.array_str(IDS_MEAN*GAIN) + " V"
			#print "MEAN IDS = " + np.array_str(1e6*IDS_MEAN) + " uA"
			#print "MAX IDS = " + np.array_str(1e6*np.amax(data)) + " uA"
			#print "MIN IDS = " + np.array_str(1e6*np.amin(data)) + " uA"
			#print "R = " + np.array_str(VDS/IDS_MEAN) + " Ohms"
			#print "VDS_bin = " + np.array_str(VDS/BITS_TO_V) + " bits"
			#print "VMID = " + np.array_str(np.mean(VMID)) + " bits"
			#print "Time = " + np.array_str(time) + " ms"

			#print "VDS_bin_real = " + np.array_str(VDS/BITS_TO_V+VMID)
			#print ("\n")

			if(data_3d.size == 0 and np.mean(data) != 0):
				time_stamps = time
				VDS_data = VDS
				VGS_data = VGS
				data_3d = data
				data_baseline = data
				flat_baseline = data_baseline.reshape(1024)
				bad_inds_low = np.where(1e6*flat_baseline < YIELD_THRESHOLD_low)
				bad_inds_high = np.where(1e6*flat_baseline > YIELD_THRESHOLD_high )
				bad_inds = np.append(bad_inds_low, bad_inds_high)
				sample_size = 1024 - np.size(bad_inds)
			elif(data_baseline.size != 0):
				time_stamps = np.append(time_stamps,time)
				VDS_data = np.append(VDS_data,VDS)
				VGS_data = np.append(VGS_data,VGS)
				data_3d = np.dstack((data_3d,data))

				#IDS_MEAN = 1e6*np.mean(data_3d, axis=2)	# average current across time in uA 
				#bool_array = IDS_MEAN > YIELD_THRESHOLD_low
				#device_yield = 100*np.true_divide(np.sum(bool_array), 256)
				#print("Yield = ", device_yield, " %")
				#print "Sample Size = " + str(sample_size)

				flat_data_2d = data_3d.reshape((1024,np.shape(data_3d)[2]))
				trunc_data_2d = np.delete(flat_data_2d, bad_inds, axis=0)
				device_yield = 100*np.true_divide(np.shape(trunc_data_2d)[0], 1024)
				print("Yield = ", device_yield, " %")

			

				dbaseline = np.delete(flat_baseline, bad_inds, axis=0)
				dbaseline = np.expand_dims(dbaseline, axis=1)
				dbaseline = np.repeat(dbaseline, np.shape(data_3d)[2], axis=1)

				dIDS = trunc_data_2d - dbaseline
				percent_change_IDS = 100*np.true_divide(dIDS, dbaseline)
				mu_array = np.mean(percent_change_IDS, axis=0)
				area3 = np.append(np.arange(0, 8, 1, dtype=int),
					np.arange(80, 87, 1, dtype=int))
				area4 = np.append(np.arange(8, 15, 1, dtype=int),
					np.arange(87, 95, 1, dtype=int))
				area1 = np.append(np.arange(144, 151, 1, dtype=int),
					np.arange(240, 246, 1, dtype=int))
				area2 = np.append(np.arange(152, 159, 1, dtype=int),
					np.arange(248, 255, 1, dtype=int))
				print(area1)
				avg_array1 = np.mean(flat_data_2d[area1], axis=0) #left up
				avg_array2 = np.mean(flat_data_2d[area2], axis=0) #right up
				avg_array3 = np.mean(flat_data_2d[area3], axis=0) #left down
				avg_array4 = np.mean(flat_data_2d[area4], axis=0) #right down
				

				errbar_array = np.std(percent_change_IDS, axis=0)
				print('data')
				print(data)
				print(np.max(data))
				print('data_3d')
				print(data_3d)
				print(np.max(data_3d))

				plt.cla()
				plt.grid(True)
				plt.title(r'$\mathrm{V_{DS}\ =\ %0.4f\ V}$' %VDS_data[0], y=1.01, fontsize=FONTSIZE)
				plt.xlabel(r'$\mathrm{Time\ (s)}$', fontsize=FONTSIZE, labelpad=XLABELPAD)
				#plt.ylabel(r'$\mathrm{\Delta I_{DS}/\overline{I_{DS}}\ (\%)}$', labelpad=YLABELPAD)
				plt.ylabel(r'$\mathrm{I_{DS}\ (\mu A)}$', labelpad=YLABELPAD)
				plt.plot(1e-3*time_stamps,1e6*avg_array1, '-r', linewidth=LINEWIDTH)
				plt.plot(1e-3*time_stamps,1e6*avg_array2, '-y', linewidth=LINEWIDTH)
				plt.plot(1e-3*time_stamps,1e6*avg_array3, '-b', linewidth=LINEWIDTH)
				plt.plot(1e-3*time_stamps,1e6*avg_array4, '-g', linewidth=LINEWIDTH)
				#print('average current = ',1e6*avg_array[-1],'uA')
				#print('i=', np.int(data_3d.shape[0] / 2))
				print('time:' ,1e-3*time_stamps[-1])
				#for i in range(np.int(np.shape(trunc_data_2d)[0] / 2)):
				#for i in range(256):
				#	if(i % 8 == 0):													# reduce number of lines to speed up plotting, make sure python doesn't lag behind actual measurement
				#		plt.plot(1e-3*time_stamps,1e6*trunc_data_2d[i,:], linewidth=LINEWIDTH)
				#plt.plot(1e-3*time_stamps,1e6*trunc_data_2d[40,:], linewidth=LINEWIDTH)

			

				#plt.fill_between(1e-3*time_stamps,mu_array-errbar_array,mu_array+errbar_array, facecolor='#089FFF', edgecolor=None, alpha=0.2)
				#fig.canvas.draw()
				plt.draw()
				plt.pause(0.0001) 

		

				if(os.path.isfile(data_file_name)):	# remove previous calibration record (for some reason the save function doesn't simply overwrite)
					os.remove(data_file_name)
				np.savez(data_file_name, time_stamps=time_stamps, VDS_data=VDS_data, VGS_data=VGS_data, data_3d=data_3d)

		except KeyboardInterrupt:
			#print "\n\n1. Enter Descriptive Data File Name: ",
			#data_file_name = raw_input()

			if(os.path.isfile(data_file_name)):	# remove previous calibration record (for some reason the save function doesn't simply overwrite)
				os.remove(data_file_name)
			np.savez(data_file_name, time_stamps=time_stamps, VDS_2d=VDS_data, VGS_2d=VGS_data, data_3d=data_3d)
			quit()	



if __name__ == '__main__':
    run()

