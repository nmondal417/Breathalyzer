//hardware: connect either VDS_RAW or one of the VGS_RAW to VMEAS for this test, depending on which channel is being tested
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

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

const int dacSelectPin = 16;
const int adcSelectPin = 2;

void setupDac();
void writeDac(unsigned int chan, unsigned int val);
float readAdc();


void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  delay(500); //wait a bit (100 ms)

  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(adcSelectPin, HIGH);
  
  setupDac();
}

void loop() {
  for (int i = 0; i <= 10; i++) {
    
    writeDac(4, i*400); //change first number (channel) to test all DAC channels
    Serial.println();
    Serial.print("DAC output: ");
    Serial.println((i*400.0)/4096.0*2.0);
    
    Serial.print("ADC reading: ");
    Serial.println(readAdc()/4096.0*2.0);
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

//Note: The ADC (AD7274) operates in SPI Mode 0 (data is sampled on the rising edge of the clock)
float readAdc() {
  word adc_stream;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(adcSelectPin, LOW);
  adc_stream = vspi->transfer16(0);   //read ADC data
  digitalWrite(adcSelectPin, HIGH);
  vspi->endTransaction();
  Serial.print("ADC stream: ");
  Serial.println(adc_stream, BIN);
  return (~(0b11 << 14) & adc_stream) >> 2;   //ignore the first two bits and the last two bits of the adc data
}