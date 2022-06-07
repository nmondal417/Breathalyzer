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
const int ldac = 4;
const int vgs_dac_channels [6] = {0, 1, 2, 3, 5, 6};   //address bits for gates 1-6 of the DAC
const float amps [3] = {1, 10.1, 20.1};
const int desired_amps [6] = {0, 0, 0, 0, 0, 0};    //0 for x1, 1 for x10, 2 for x20

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
  digitalWrite(ldac, HIGH);
  digitalWrite(dacSelectPin, HIGH);
  
  setupDac();
}

void loop() {
  for (int i = 0; i <= 10; i++) {
    Serial.println();
    for (int gate = 0; gate < 6; gate++) {
      
      int voltage_binary = i*400;
      int offset = 0;
      if ((gate == 4 || gate == 5) && (voltage_binary >= 20)) {
        offset = 20;
      }
      
      float true_voltage = ((voltage_binary/4096.0*2.0 - 1)*amps[desired_amps[gate]]) + 1;
      writeDac(vgs_dac_channels[gate], voltage_binary - offset); 
      Serial.println("Gate " + String(gate+1) + ": " + String(true_voltage));
    }
    ldac_pulse();
    delay(4000);
  }

}
