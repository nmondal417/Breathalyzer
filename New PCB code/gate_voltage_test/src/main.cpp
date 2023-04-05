//Probe voltages and compare with expected voltages
//DAC channels:
//  0: VGS2_RAW
//  1: VGS1_RAW
//  2: VGS3_RAW
//  3: VDS_RAW
//  4: VGS4_RAW
//  5: VGS6_RAW
//  6: VGS5_RAW

#include <Arduino.h>
#include <SPI.h>

#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK

#if CONFIG_IDF_TARGET_ESP32S2
#define VSPI FSPI
#endif

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

const int dacSelectPin = 16;
const int chip_en = 33;
const int gate_amp = 32;
const int gate_shdn = 22;

const int vgs_dac_channels [6] = {1, 0, 2, 4, 6, 5};   //address bits for gates 1-6 of the DAC
const bool amp = false;     //true for x10 amplification, false for x1

void setupDac();
void writeDac(unsigned int chan, unsigned int val);
float readAdc();

void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  delay(500); //wait a bit (100 ms)

  pinMode(dacSelectPin, OUTPUT);
  pinMode(chip_en, OUTPUT);
  pinMode(gate_amp, OUTPUT);
  pinMode(gate_shdn, OUTPUT);

  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(gate_shdn, LOW);
  delay(4000);

  //IMPORTANT: set amplification and turn on gate switch before turning on the gate converter,
  // to ensure that proper gate voltages are outputted
  
  if (amp) digitalWrite(gate_amp, HIGH);   //set amplification of gates
  else digitalWrite(gate_amp, LOW);
  digitalWrite(chip_en, LOW);              //enable the gate switch
  delay(4000);

  digitalWrite(gate_shdn, HIGH);           //turn gate voltage converter on

  setupDac();
}

void loop() {
  for (int i = 0; i <= 10; i++) {
    Serial.println();
    for (int gate = 0; gate < 6; gate++) {
      
      int voltage_binary = i*400;
      float true_voltage;

      if (amp) true_voltage = ((voltage_binary/4096.0*2.0 - 1)*10.1) + 1; 
      else true_voltage = voltage_binary/4096.0*2.0;
      writeDac(vgs_dac_channels[gate], voltage_binary); 
      Serial.println("Gate " + String(gate+1) + ": " + String(true_voltage));
    }
    delay(4000);
  }

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