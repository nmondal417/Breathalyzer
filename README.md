# Breathalyzer Project Overview

The overall goal of this project is to creaate a wireless device that can be used to analyze the breath of medical patients to diagnose various diseases. The underlying technology is a graphene FET array. When the graphene FETs come into contact with different gases, the properties of the FETs will change. For example, the IDS vs VGS curve of the graphene FET may shift depending on the concentration of a certain gas molecule in the patient's breath. 

This GitHub project contains the PCBs and the code that are used to control and measure the graphene FET array. In simple terms, the PCB generates analog voltages VDS and VGS, and sends these to the graphene FET. Two multiplexers are used to select one particular FET out of the array. The PCB then measures the resulting IDS on that FET. This process is then repeated for every FET in the array, and then for different VDS and VGS. A python script can be used to plot the various voltage vs current curves.

![functional_diagram](https://github.com/nmondal417/Breathalyzer/assets/59549888/d741f6c7-2868-4da5-ab92-3896cb13cb0b)

# Device Design

The device is split into two chambers. The top chamber contains the graphene FET chip, an interface PCB for connecting to the chip, a humdity sensor, and a heater to prevent condensation on the chip. The bottom chamber contains most of the control electronics, inlucding the ESP32 micronctroller, the amplifiers, and multiplexers. There is also a single-cell 2 Ah LiPo for powering all the electronics, and a boost converter for specifically powering the heater. The LiPo can be charged through the USB-C connector located on the bottom of the device; when charging, the device will automatically turn off. 

![breathalyzer_drawing](https://github.com/nmondal417/Breathalyzer/assets/59549888/a66174f2-f314-4a3d-9204-694ee9273c8a)

On the outside of the device, there are various switches to control and LEDs to monitor the breathalyzer status. The main on switch connects the LiPo to the system and turns everything except the heater on; the heater on switch controls the heater. An RGB charge status LED lights up when the device is being charged; an orange light indicates that charging is occurring, while a white light indicates that charging is completed. Finally a "BLOW" LED controlled by the ESP32 indicates to the patient when they should blow into the device.

# PCBs

## ESP32 Carrier

This PCB is where the ESP32, the main microcontroller, mounts **(NOTE: make sure to place the ESP32 correctly! the USB conenctor should face the side labeled "FRONT")**. The ESP32 controls the DAC, the ADC, and the multiplexers. It can also read data from the humidity sensor and the heater thermistor. It reports the current measurements in either serial or Bluetooth serial. 

The carrier board also contains the LiPo charge controller, the USB-C charging port, and the main 3.3V regulator that powers most of the ICs. All of the buttons and switches plug into the ESP32 carrrier board.


