# Breathalyzer Project Overview

The overall goal of this project is to create a wireless device that can be used to analyze the breath of medical patients to diagnose various diseases. The underlying technology is a graphene FET array. When the graphene FETs come into contact with different gases, the properties of the FETs will change. For example, the IDS vs VGS curve of the graphene FET may shift depending on the concentration of a certain gas molecule in the patient's breath. 

This GitHub project contains the PCBs and the code that are used to control and measure the graphene FET array. In simple terms, the PCB generates analog voltages VDS and VGS, and sends these to the graphene FET. Two multiplexers are used to select one particular FET out of the array. The PCB then measures the resulting IDS on that FET. This process is then repeated for every FET in the array, and then for different VDS and VGS. A python script can be used to plot the various voltage vs current curves.

![functional_diagram](https://github.com/nmondal417/Breathalyzer/assets/59549888/d741f6c7-2868-4da5-ab92-3896cb13cb0b)

# Device Design

The device is split into two chambers. The top chamber contains the graphene FET chip, an interface PCB for connecting to the chip, a humdity sensor, and a heater to prevent condensation on the chip. The bottom chamber contains most of the control electronics, inlucding the ESP32 micronctroller, the amplifiers, and multiplexers. There is also a single-cell 2 Ah LiPo for powering all the electronics, and a boost converter for specifically powering the heater. The LiPo can be charged through the USB-C connector located on the bottom of the device; when charging, the device will automatically turn off. 

![breathalyzer_drawing](https://github.com/nmondal417/Breathalyzer/assets/59549888/a66174f2-f314-4a3d-9204-694ee9273c8a)

On the outside of the device, there are various switches to control and LEDs to monitor the breathalyzer status. The main on switch connects the LiPo to the system and turns everything except the heater on; the heater on switch controls the heater. An RGB charge status LED lights up when the device is being charged; an orange light indicates that charging is occurring, while a white light indicates that charging is completed. Finally a "BLOW" LED controlled by the ESP32 indicates to the patient when they should blow into the device.

# PCBs

## ESP32 Carrier

https://github.com/nmondal417/Breathalyzer/tree/main/New%20PCB/Breathalyzer_ESP32

This PCB is where the ESP32, the main microcontroller, mounts **(NOTE: make sure to place the ESP32 correctly! the USB connector should face the side labeled "FRONT")**. The ESP32 controls the DAC, the ADC, and the multiplexers. It can also read data from the humidity sensor and the heater thermistor. It reports the current measurements in either serial or Bluetooth serial. 

The carrier board also contains the LiPo charge controller, the USB-C charging port, and the main 3.3V regulator that powers most of the ICs. All of the buttons and switches plug into the ESP32 carrrier board.

## Amplifier

https://github.com/nmondal417/Breathalyzer/tree/main/New%20PCB/Breathalyzer_Power%2BICs

This PCB contains the DAC, the ADC, and the op-amps that amplify the gate voltages.

The DAC (AD5328) communicates with the ESP32 over SPI. The DAC generates 6 gate voltages and 1 drain voltage. These voltages are in the range of 0 to 2 V; however, since the source voltage of this system is 1 V, the VGS and VDS voltages are actually -1 to +1 V. 

The op-amps then buffer and amplify the gate voltages. The op-amps can either provide x1 amplification (where it just acts a buffer), or x10 amplifcation. This amplification is controlled by the ESP32. The ESP32 controls two solid-state switches (ADG5433), which can change the amplifer gain by switching in a feedback resistor connected across the op-amp. The post-amplifcation gate voltage can be from -10 to +10 V.

The ADC (AD7274) communicates with the ESP32 over SPI. The ADC input is the voltage from the transimpedance amplifier. The ADC reads in range of 0 to 2V, and sends the ESP32 a 12-bit digital reading. 

This PCB also contains the 1V and 2V regulators that generate reference voltages for the DAC, ADC, and the op-amps, as well as a dual DC/DC converter (LT1945) that generates the -11V and +11V rails used for the amplifiers.

Finally, there is a mechanical switch that can completely disconnect the gate voltages from the graphene chip. This is useful for quickly turning off a voltage sweep, since if the incorrect gate voltages are accidentally applied the FET array can be destroyed.

## Multiplexer

https://github.com/nmondal417/Breathalyzer/tree/main/New%20PCB/Breathalyzer_Mux%2BConn

This PCB contains the two main multiplexers, the ribbon cable connector, and the transimpedance amplifier. 

The multiplexers are the ADG732. One is the drain demultiplexer, which sends the drain voltage to 1 of the 32 rows of the FET array. The other is the source muliplexer, which measures the source current from 1 of the 32 columns. 

The ribbon cable connector connects to the ribbon cable that runs from this PCB to the chip interface board. This cable is the PCB's connection to the drain, gate, and source terminals on the FET array.

The transimpedance ampliifer is an op-amp that is used to convert the source current measurement into a voltage. The formula is VMEAS = 1 - 1000*IDS, so an IDS of 100 uA would result in a VMEAS of 0.9V. This VMEAS is sent to the ADC.

# Code

All the code for this version of the breathlyzer is under 'New PCB code.' While the main code that runs on the ESP32 during normal operation is IV_Array_main_NEW, there are other files that test specific parts of the system, like the DAC and ADC, and are helpful for debugging.

## Blink

Simple test to just check if code can be uploaded correctly to the ESP32.

## ADC_DAC_test

This test can be used to test the ADC, DAC, or both together. Bascially, write analog voltages using the DAC and read an analog voltage using the ADC. If you are testing both the DAC and ADC at the same time, connect one of the DAC's output voltages to the input of the ADC.

## offset_and_gain_test

Used for checking and adjusting for the offset or gain error of the ADC and DAC. Test each DAC channel by connecting it to the ADC, and compare the read voltages to the expected values.

## gate_voltage_test

Tests that gate voltages can be generated properly. Combines the DAC code along with additional code to turn on the gate amplifier power supply and control the amplification.

## multiplexer_test

Close to a full system test, but instead of having the actual graphene array connected, you connect a test resistor between a drain connection on one mux and source connection on the other. When the correct select bits are sent to the muxes, the code should report the resistor value. 



