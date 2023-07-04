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

This PCB is where the ESP32, the main microcontroller, mounts **(NOTE: make sure to place the ESP32 correctly! the USB conenctor should face the side labeled "FRONT")**. The ESP32 controls the DAC, the ADC, and the multiplexers. It can also read data from the humidity sensor and the heater thermistor. It reports the current measurements in either serial or Bluetooth serial. 

The carrier board also contains the LiPo charge controller, the USB-C charging port, and the main 3.3V regulator that powers most of the ICs. All of the buttons and switches plug into the ESP32 carrrier board.

## Amplifier

https://github.com/nmondal417/Breathalyzer/tree/main/New%20PCB/Breathalyzer_Power%2BICs

This PCB contains the DAC, the ADC, and the op-amps that amplify the gate voltages.

The DAC (AD5328) communicates with the ESP32 over SPI. The DAC generates 6 gate voltages and 1 drain voltage. These voltages are in the range of 0 to 2 V; however, since the source voltage of this system is 1 V, the VGS and VDS voltages are actually -1 to +1 V. 

The op-amps then buffer and amplify the gate voltages. The op-amps can either provide x1 amplification (where it just acts a buffer), or x10 amplifcation. This amplification is controlled by the ESP32. The ESP32 controls two solid-state switches (ADG5433), which can change the amplifer gain by switching a feedback resistor connected across the op-amp. Therefore, the post-amplifcation gate voltage can be from -10 to +10 V.

The ADC (AD7274) communicates with the ESP32 over SPI. The ADC input is the voltage from the transimpedance amplifier. The ADC reads in range of 0 to 2V, and sends the ESP32 a 12-bit digital reading. 

This PCB also contains the 1V and 2V regulators that generate reference voltages for the DAC, ADC, and the op-amps, as well as a dual DC/DC converter (LT1945) that generates the -11V and +11V rails used for the amplifiers.

Finally, there is a mechanical switch that can completely disconnect the gate voltages from the graphene chip. This is useful for quickly turning off a voltage sweep, since if the incorrect gate voltages are accidentally applied the FET array can be destroyed.




