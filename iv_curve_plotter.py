# generates IV curves for chemical sensors
#!/usr/bin/env python

# VMID (LDO1) = 1.6441 V
# VSUPPLY (LDO1) = 3.3098 V

import os
import sys
import re
import matplotlib

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
import numpy as np
import serial
import time as timer

s = serial.Serial(port='/dev/cu.usbmodem14101', baudrate=115200)# Arduino DUE
#s = serial.Serial(port='/dev/cu.usbmodem14201', baudrate=115200) #right-side USB
#s = serial.Serial(port='/dev/cu.usbmodem1411', baudrate=230400)	# Arduino DUE


GAIN = 1000
FONTSIZE = 20           # fontsize for plotss
LINEWIDTH = 1.5
XLABELPAD = 5           # pad the xlabel away from the xtick labels
YLABELPAD = 10          # pad the ylabel away from the ytick labels

'''
VMID = 1
VMID_bin = 2048
V_SUPPLY_PCB = 3.2965
V_SUPPLY_ARDUINO = 3.2662
BITS_TO_V = V_SUPPLY_ARDUINO/4095
'''

def get_data(time, VDS, VGS, VMID, data, R, string):	
	string = s.readline()

	#print('string list is :',string)
	#print(len(string.decode().split(',')))
	if(len(string.decode().split(','))%260 == 0):
		data = np.fromstring(string, dtype=float, sep=',')		# string to array
		time = data[0]
		VDS = data[1]
		VGS = data[2:7]
		data = data[8:]
		data = data/GAIN
		data = data.reshape(256,1)							# reshape array to 16 x 16
		VDS_array = np.ones((256,1))*VDS
		#R = np.true_divide(VDS_array,data)
		R = 0
	return [time, VDS, VGS, VMID, data, R, string]

def run():
	RECORD = np.empty((1,0))
	time = np.empty((1,0))
	VDS = np.empty((1,0))
	VGS = np.empty((1,0))
	VMID = np.empty((1,0))
	R = np.zeros((16,16))
	time_stamps = np.empty((1,0))
	data = np.empty((256,0))
	string = np.empty((0,0))
	plt.ion()										# interactive plotting
	plt.show() 										# show plot		
	fig = plt.figure(figsize=(10,10))				# create figure for plot
	plt.grid(True)
	ax = plt.gca()
	plt.xlabel(r'$\mathrm{V_{GS}\ (V)}$', fontsize=FONTSIZE, labelpad=XLABELPAD)
	plt.ylabel(r'$\mathrm{I_{DS}\ (\mu A)}$', fontsize=FONTSIZE, labelpad=YLABELPAD)
	plt.pause(0.0001)                                #fix plt.ion frozen window 


	while True:
		
		try:
			time, VDS, VGS, VMID, data, R, string = get_data(time, VDS, VGS, VMID, data, R, string)		# get ADC data array

			if('END' in str(string) and RECORD == 1):
			#if string.decode().count('END') > 0 and RECORD == 1:
				print ("\n\n1. Enter Descriptive Data File Name: "),
				data_file_name = input()
				
				#if(os.path.isfile('iv_data.npz')):	# remove previous calibration record (for some reason the save function doesn't simply overwrite)
				#	os.remove('iv_data.npz')
				#np.savez('iv_data', time_stamps=time_stamps, VDS_2d=VDS_2d, VGS_2d=VGS_2d, data_3d=data_3d)

				if os.path.isfile(data_file_name):	# remove previous calibration record (for some reason the save function doesn't simply overwrite)
					os.remove(data_file_name)
				np.savez(data_file_name, time_stamps=time_stamps, VDS_2d=VDS_2d, VGS_2d=VGS_2d, data_3d=data_3d)
				#np.savetxt(data_file_name, time_stamps=time_stamps, VDS_2d=VDS_2d, VGS_2d=VGS_2d, data_3d=data_3d) #save as txt file 



				print(np.amax(data_3d))
				print(np.amin(data_3d))
				quit()

			if(RECORD):
				

				print("VDS = %0.2f mV,  VGS = %0.2f  mV,  VMID_bin = %0.2f bits" %(1e3*VDS, 1e3*VGS, VMID))

				time_stamps = np.append(time_stamps,time)

				data_2d = np.hstack((data_2d,data))		# add data to 2d array
				VGS_1d = np.append(VGS_1d,VGS)
				VDS_1d = np.append(VDS_1d,VDS)

				plt.title(r'$\mathrm{V_{DS}\ =\ %0.4f\ V}$' %VDS, y=1.01, fontsize=FONTSIZE) 
				for i in range(np.int(data_3d.shape[0] / 2)):
					if(i % 8 == 0):													# reduce number of lines to speed up plotting, make sure python doesn't lag behind actual measurement
						plt.plot(VGS_1d,1e6*data_2d[i,:], linewidth=LINEWIDTH)
				plt.draw()
				#fig.canvas.draw()								# works as an alternative to plt.draw(), not sure why though		
				#fig.canvas.flush_events()
				plt.pause(0.0001) 

				vgs_cnt = vgs_cnt + 1

				if(vgs_cnt == vgs_dim):
					vgs_cnt = 0
					vds_cnt = vds_cnt + 1
					data_3d = np.dstack((data_3d,data_2d))	# append to 3d array
					data_2d = np.empty((256,0))				# clear 2d array
					VGS_2d = np.hstack((VGS_2d,VGS_1d.reshape(VGS_1d.shape[0],1)))		# make 2D for stacking VGS data
					VDS_2d = np.hstack((VDS_2d,VDS_1d.reshape(VDS_1d.shape[0],1)))		# make 2D for stacking VDS data
					VGS_1d = np.empty((1,0))				# clear VGS 1d array
					VDS_1d = np.empty((1,0))				# clear VDS 1d array
					plt.close("all")
					plt.ion()										# interactive plotting
					plt.show() 										# show plot		
					fig = plt.figure(figsize=(10,10))				# create figure for plot
					plt.grid(True)
					ax = plt.gca()
					plt.xlabel(r'$\mathrm{V_{GS}\ (V)}$', fontsize=FONTSIZE, labelpad=XLABELPAD)
					plt.ylabel(r'$\mathrm{I_{DS}\ (\mu A)}$', fontsize=FONTSIZE, labelpad=YLABELPAD)
					#plt.ylim(0,175)
					plt.pause(0.0001) 

			#if(len(string.decode().split(',')) == 3 and ''string.decode().count('256') > 0 ''):
			if(len(string.decode().split(',')) == 3 and '256' in str(string)):
				data = np.fromstring(string, dtype=int, sep=',')
				fet_dim = data[0]
				vgs_dim = data[1]
				vds_dim = data[2]
				vgs_cnt = 0
				vds_cnt = 0
				VGS_2d = np.empty((vgs_dim,0))			# VGS array is now 2D because it will vary a little every cycle
				VDS_2d = np.empty((vgs_dim,0))			# VDS array is now 2D because it will vary a little every cycle
				data_2d = np.empty((256,0))
				data_3d = np.empty((fet_dim,vgs_dim,0))
				VDS_1d = np.empty((1,0))
				VGS_1d = np.empty((1,0))
				RECORD = 1
			
		except KeyboardInterrupt:
			quit()


if __name__ == '__main__':
    run()

