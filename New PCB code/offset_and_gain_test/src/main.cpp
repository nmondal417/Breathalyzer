//test for checking the offset and gain error of the ADC and/or the DAC
//connect one of the DAC channels to VMEAS, the ADC input

#include <Arduino.h>
#include <SPI.h>

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

const int dacSelectPin = 5;
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
  for (int dac = 0; dac < 8; dac++) {
    writeDac(dac, 0);
  }

  delay(10);
  
  Serial.print("ADC reading at ideal 0V: ");
  Serial.println(readAdc()/4096.0*2.0, 4);
  
  delay(2000);
  for (int dac = 0; dac < 8; dac++) {
    writeDac(dac, 4095);
  }
  delay(10);
  Serial.print("ADC reading at ideal 2V: ");
  Serial.println(readAdc()/4096.0*2.0, 4);
  delay(2000);

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