#include <Arduino.h>
#include <SPI.h>
#include <string.h>

#include "BluetoothSerial.h"

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

//BME setup stuff
#define BME_CS 33
Adafruit_BME680 bme(BME_CS);

BluetoothSerial SerialBT; 

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000;  //SPI clock frequency for DAC and ADC

//ADC and DAC pins
const int dacSelectPin = 16;    //slave select for DAC
const int adcSelectPin = 2;    //slave select for ADC
const int vgs_dac_channels [6] = {1, 0, 2, 4, 6, 5};   //DAC address bits for gate voltages 1-6
const int vds_dac_channel = 3; //channel for drain voltage
const bool amp = false;     //true for x10 gate voltage amplification, false for x1

//sweep parameters
float VGS_SWEEP_RATE = 0; //VGS sweep rate in ms
float VDS_HOLD = 0;      //VDS_hold at beginning of sweep
int numReadings = 100;  //number of readings taken for each FET in the array
int numDummy = 5;      //number of dummy reads done beforehand
float VMID = 1;       //offset voltage for VGS and VDS

//Vgs parameters for gates 1 through 6
float VGS_START [6] = {-0.5, -0.5, -0.5, -0.5, -0.5, -0.5};       
float VGS_STOP [6] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
float VGS_INC [6] = {10e-3, 10e-3, 10e-3, 10e-3, 10e-3, 10e-3};
float VGS_OFFSETS [6] = {0, 0, 0, 0, 0, 0};  //positive offset means the DAC outputs a higher-than-expected value
float vgs [6];    //VGS as a value from -1 to 1V
int vgs_bin [6];  //VGS as a value from 0 to 4095

//Vds parameters
const float VDS_START = 800e-3;
const float VDS_STOP = 800e-3;
const float VDS_INC = 5e-3;
float VDS_OFFSET = 0; //positive offset means the DAC outputs a higher-than-expected value
float vds;     //VDS as a value from -1 to 1V
int vds_bin;   //VDS as a value from 0 to 4095 (0V = 0, 2V = 4095)

//source and drain mux pins
int drain_sel [5] = {12, 0, 14, 15, 5};   //pin 12 is the LSB, pin 5 is the MSB
int source_sel [5] = {21, 4, 25, 26, 27};  //pin 21 is the LSB, pin 27 is the MSB
const int multiplexer_max = 32;           //number of rows and columns on the chip

const int chip_en = 33;    //must be brought low to enable the multiplexers and the gate switch
const int gate_amp = 32;   //bring LOW for x1 amplification, bring HIGH for x10 amplification
const int gate_shdn = 22;  //bring HIGH to enable gate voltage converter

String message;    //Serial print message

char bluetooth_inst [10] = "";   //array to hold instruction received from Bluetooth serial

int powers_of_two [5] = {1, 2, 4, 8, 16};

//function definitions
String sweep(float vds, float* vgs);
void setupDac();
void writeDac(unsigned int chan, unsigned int val);
float readAdc();
int v_to_bin(float voltage);
float bin_to_v(int bin);
void multiplexer(int address, bool drain_or_source);
void copy(float* src, float* dst, int len);

void setup() {
  
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair with bluetooth!");
  //delay(1000); //wait a bit (1000 ms)

  //CS pins for ADC and DAC
  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(adcSelectPin, HIGH);

  //Address pins for the two multiplexers
  for (int i = 0; i < 5; i++) {
    pinMode(drain_sel[i], OUTPUT);
    pinMode(source_sel[i], OUTPUT);
  }
  
  for (int i = 0; i < 5; i++) {
    digitalWrite(drain_sel[i], LOW);
    digitalWrite(source_sel[i], LOW);
  }

  pinMode(chip_en, OUTPUT); 
  pinMode(gate_amp, OUTPUT); 
  pinMode(gate_shdn, OUTPUT);

  digitalWrite(gate_shdn, LOW); //turn off the gate voltage converter
  delay(4000);

  //IMPORTANT: set amplification and turn on gate switch before turning on the gate voltage converter,
  // to ensure that proper gate voltages are outputted
  
  if (amp) digitalWrite(gate_amp, HIGH);   //set amplification of gates
  else digitalWrite(gate_amp, LOW);
  digitalWrite(chip_en, LOW);              //enable the gate switch
  delay(4000);

  digitalWrite(gate_shdn, HIGH);           //turn gate voltage converter on
  
  
  setupDac();

  //BME680 setup
  while (!Serial);
  Serial.println(F("BME680 test"));
  /*
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  } */

  // BME680: Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

}

void loop() {
  /*
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *C");

  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %"); */

  memset(bluetooth_inst, 0, sizeof(bluetooth_inst));  //clear bluetooth_inst 

  byte i = 0;
  while(SerialBT.available()) {  //read from bluetooth serial
    char letter = SerialBT.read();
    if (letter != '\r' && letter != '\n' && i < sizeof(bluetooth_inst)) {
      bluetooth_inst[i] = letter;
      i++;
    }
  }
  delay(1000);

  if (i != 0) {       //print received message
    Serial.print("I received: ");
    Serial.println(bluetooth_inst);
  }

  if (strcmp(bluetooth_inst, "start") == 0) {    //start program is valid message received
    Serial.println("starting");


    int i = 0;
    int j = 0;
    for(vgs[0] = VGS_START[0]; vgs[0] <= VGS_STOP[0]+0.5*VGS_INC[0]; vgs[0] = vgs[0] + VGS_INC[0]) {i++;} // determining data array size for exporting
    for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {j++;}
    Serial.println("256," + String(i) + "," + String(j));

    //outer loop for Vds sweep, inner loop for Vgs sweep
    for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {    // added 0.5*VDS_INC just in case VDS is not exactly precise and the loop terminates early
      
      vds_bin = v_to_bin(vds - VDS_OFFSET);  // convert desired voltage into appropriate binary value (0 to 4095)
      writeDac(vds_dac_channel, vds_bin);

      for (int gate = 0; gate < 6; gate++) {
        vgs[gate] = VGS_START[gate];
      }
      
      while(vgs[0] <= VGS_STOP[0]+0.5*VGS_INC[0]) {  // added 0.5*VGS_INC just in case VGS is not exactly precise and the loop terminates early

        for (int gate = 0; gate < 6; gate++) {
          vgs_bin[gate] = v_to_bin(vgs[gate] - VGS_OFFSETS[gate]);             // convert desired voltage into appropriate binary value (0 to 4095)
          writeDac(vgs_dac_channels[gate], vgs_bin[gate]);                    // write DAC value to 1 of the 6 gates
        }
        
        
        if(vgs[0] == VGS_START[0]) {delay(VDS_HOLD);}
        else {delay(VGS_SWEEP_RATE);}                             // delay to allow EDL to reach steady state

        message = sweep(vds,vgs);
        Serial.println(message); 

        SerialBT.println(message);
        /*
        if (! bme.performReading()) {
          SerialBT.println("Failed to perform reading :(");
          return;
        }
        
        SerialBT.print("Temperature = ");
        SerialBT.print(bme.temperature);
        SerialBT.println(" *C");

        SerialBT.print("Humidity = ");
        SerialBT.print(bme.humidity);
        SerialBT.println(" %"); */


        memset(bluetooth_inst, 0, sizeof(bluetooth_inst));
        byte i = 0;
        while(SerialBT.available()) {
          char letter = SerialBT.read();
          if (letter != '\r' && letter != '\n' && i < sizeof(bluetooth_inst)) {
            bluetooth_inst[i] = letter;
            i++;
          }
        }

        if (strcmp(bluetooth_inst, "stop") == 0) { //stop and exit sweep
          Serial.println("stopping");
          break;
        }

        for (int gate = 0; gate < 6; gate++) {
          vgs[gate] += VGS_INC[gate];
        }
        
        delay(5000);
      }
    }

    Serial.println("END");
  }
}

String sweep(float vds, float* vgs) {
  unsigned long t1;
  int vmeas_bin_avg = 0;
  int vmeas_bin = 0;
    
  String message = "";

  t1 = millis();
  message += String(t1);
  
  message += ",";
  message += String(vds,4);                        // gives VDS with 4 decimals of precision
  message += ",";

  for (int gate = 0; gate < 6; gate++) {
    float vgs_amped = amp ? vgs[gate]*10.1:
                            vgs[gate];
    message += String(vgs_amped,4);                        // gives VGS with 4 decimals of precision
    message += ",";
    break;   //only once for now
  }
  
  message += String(VMID);
  message += ",";
  
  for (int vds_select = 0; vds_select < multiplexer_max; vds_select++) {   
    for (int vmeas_select = 0; vmeas_select < multiplexer_max; vmeas_select++) {      
      multiplexer(vds_select, 0);
      multiplexer(vmeas_select, 1);
      
      vmeas_bin_avg = 0;
      
      for (int i = 0; i < numDummy; i++) { //perform some dummy reads to let the currents settle
        readAdc();   
      }  

      for (int i = 0; i < numReadings; i++) { //actual readings
        vmeas_bin = readAdc();
        vmeas_bin_avg += vmeas_bin; 
      }
      
      vmeas_bin_avg = vmeas_bin_avg / numReadings;

      //for 0 to 4095 integer output
      //message += String(vmeas_bin_avg);

      //for -1 to 1 float output
      float vmeas_avg = bin_to_v(vmeas_bin_avg);
      message += String(vmeas_avg, 4);

      message += ",";
    }
  }
  message = message.substring(0, message.length() - 1);  // removes the last comma so that when comma delimited we have an array with 256 elements and not 257
  message += "\n";
  return message;
}

//Note: The DAC (AD5328) operates in SPI Mode 1 (data is sampled on the falling edge of the clock)
void setupDac() {
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(0b1 << 15);   //set up DAC with gain of 1, unbuffered on all channels
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();


  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(0b1010000000000000);   //set up DAC with with LDAC low, DAC registers update continuously
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();
  
}

//Note: The DAC (AD5328) operates in SPI Mode 1 (data is sampled on the falling edge of the clock)
void writeDac(unsigned int chan, unsigned int val) {
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE1));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(((0b111 & chan) << 12) + val);   //bits 14 through 12 for channel select, lower 12 bits for DAC data
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();
}

//Note: The ADC (AD7274) operates in SPI Mode 0 (data is sampled on the rising edge of the clock)
float readAdc() {
  word adc_stream;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(adcSelectPin, LOW);
  adc_stream = vspi->transfer16(0);   //read ADC data
  digitalWrite(adcSelectPin, HIGH);
  vspi->endTransaction();
  //Serial.print("ADC stream: ");
  //Serial.println(adc_stream, BIN);
  return (~(0b11 << 14) & adc_stream) >> 2;   //ignore the first two bits and the last two bits of the adc data
}

//gate_or_source: 0 if drain, 1 if source
//channels are 0-indexed
void multiplexer(int address, bool drain_or_source) {
  int *select_bits;
  if (drain_or_source) {
    select_bits = source_sel;
  }
  else {
    select_bits = drain_sel;
  }

  for (int i = 4; i >= 0; i--) {
    if (address >= powers_of_two[i]) {
      digitalWrite(select_bits[i], HIGH);
      address -= powers_of_two[i]; 
    }
    else {
      digitalWrite(select_bits[i], LOW);
    }
  }
}

//converts a Vds or Vgs voltage from -1 to 1V to an integer from 0 to 4095
int v_to_bin(float voltage) {
  int bin;
  voltage = voltage + VMID;
  bin = int(voltage / (2*VMID) * 4096 );
  bin = min(max(0, bin), 4095);  //enforce limits of 12-bit binary number
  return bin;
}

//converts an integer from 0 to 4095 to a voltage from -1 to 1V
float bin_to_v(int bin) {
  float voltage;
  voltage = (bin / 4096.0 * 2*VMID) - VMID;
  voltage = min(max(-VMID, voltage), VMID); 
  return voltage;
}

// Function to copy 'len' elements from 'src' to 'dst'
void copy(float* src, float* dst, int len) {
    memcpy(dst, src, sizeof(src[0])*len);
}