#include <SPI.h>

#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK
//#define VSPI_SS     SS

#if CONFIG_IDF_TARGET_ESP32S2
#define VSPI FSPI
#endif

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

const int dacSelectPin = 5;
const int adcSelectPin = 2;
const int ldac = 4;
const int adc_esp = 15;
const int dac_esp = 26;

const float test_resistor = 2200;  //resistor used to test transimpedance amp; placed between drain and source voltages


void setupDac() {
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(0b1 << 15);   //set up DAC with gain of 1, unbuffered on all channels
  //Serial.println(0b1 << 15);
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();


  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(0b1010000000000001);   //set up DAC with with LDAC high (default mode, so not needed), DAC outputs will not update until LDAC is pulsed low
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();
  
}

void writeDac(unsigned int chan, unsigned int val) {
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(((0b111 & chan) << 12) + val);   //bits 14 through 12 for channel select, lower 12 bits for DAC data
  //Serial.println(((0b111 & chan) << 12) + val);
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();
}

float readAdc() {
  word adc_stream;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(adcSelectPin, LOW);
  adc_stream = vspi->transfer16(0);   //read ADC data
  digitalWrite(adcSelectPin, HIGH);
  vspi->endTransaction();
  //Serial.println(adc_stream);
  //Serial.println((~(0b11 << 14) & adc_stream) >> 1);
  return (~(0b11 << 14) & adc_stream) >> 1;   //ignore the first two bits and the last bit of the adc data
}

void ldac_pulse() {
  digitalWrite(ldac, LOW);
  digitalWrite(ldac, HIGH);
}


void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  delay(500); //wait a bit (100 ms)

  pinMode(ldac, OUTPUT);
  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  digitalWrite(ldac, HIGH);
  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(adcSelectPin, HIGH);

  pinMode(adc_esp, INPUT);
  pinMode(dac_esp, OUTPUT);
  analogWrite(dac_esp, 100);
  
  setupDac();
}

void loop() {
  for (int i = 0; i <= 10; i++) {

    int vds_bin = 500+ 200 * i;  //sweep Vds from 0V to 1V (offset by 1V since source voltage is 1V)
    float vds = vds_bin/4096.0*2.0;
    
    writeDac(4, vds_bin - 35);  //channel 4 is for Vds raw, offset of about 10 mV
    ldac_pulse();
    Serial.println();
    Serial.println("DAC output: " + String(vds));
    Serial.println("Expected Current: " + String((vds - 1.0)/test_resistor*1000, 4) + " mA");
    delay(1000);
    float transimp_out = readAdc()/4096.0 * 2.0;
    //Serial.println("ADC voltage reading: " + String(readAdc()/4096.0 * 2.0, 4));
    Serial.println("Measured Current: " + String((1.0 - transimp_out), 4) + " mA");
/*
    float transimp_esp = analogRead(adc_esp)/4096.0*3.3;
    Serial.println("ADC voltage reading (ESP): " + String(transimp_esp, 4));
    Serial.println("Measured Current (ESP): " + String((1.0 - transimp_esp), 4) + " mA"); */
    delay(4000); 
  }

}
