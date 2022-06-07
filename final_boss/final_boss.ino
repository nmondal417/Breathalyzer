//basically, run everything at once and see if anything breaks

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

const int vgs_dac_channels [6] = {0, 1, 2, 3, 5, 6};   //address bits for gates 1-6 of the DAC
const float amps [3] = {1, 10.1, 20.1};
const int desired_amps [6] = {2, 2, 2, 2, 2, 2};    //0 for x1, 1 for x10, 2 for x20

const float test_resistor = 1000;  //resistor used to test transimpedance amp; placed between drain and source voltages

//source and drain mux pins
int drain_sel [5] = {12, 13, 14, 15, 16};   //pin 12 is the LSB, pin 16 is the MSB
int source_sel [5] = {21, 22, 25, 26, 27};  //pin 21 is the LSB, pin 27 is the MSB
const int drain_en = 17; //must be brought low to enable drain demultiplexer
const int source_en = 32; //must be brought low to enable source multiplexers

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
    if (address >= pow(2, i)) {
      digitalWrite(select_bits[i], HIGH);
      address -= pow(2, i); 
    }
    else {
      digitalWrite(select_bits[i], LOW);
    }
  }
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

  pinMode(drain_en, OUTPUT);
  pinMode(source_en, OUTPUT);
  for (int i = 0; i < 5; i++) {
    pinMode(drain_sel[i], OUTPUT);
    pinMode(source_sel[i], OUTPUT);
  }

  digitalWrite(drain_en, LOW);  //enable the drain demultiplexer
  digitalWrite(source_en, LOW); //enable the source multiplexer
  for (int i = 0; i < 5; i++) {
    digitalWrite(drain_sel[i], LOW);
    digitalWrite(source_sel[i], LOW);
  }
  
  setupDac();
}

void loop() {
  for (int i = 0; i <= 10; i++) {

    int vds_bin = 400 * i;  //sweep Vds from 0V to 1V (offset by 1V since source voltage is 1V)
    float vds = vds_bin/4096.0*2.0;
    
    writeDac(4, vds_bin - 35);  //channel 4 is for Vds raw, offset of about 10 mV
    ldac_pulse();
    Serial.println();
    Serial.println("DAC output: " + String(vds));
    Serial.println("Expected Current: " + String((vds - 1.0)/test_resistor*1000, 4) + " mA");
    
    multiplexer(31, 0);
    multiplexer(0, 1);


    float transimp_out = readAdc()/4096.0 * 2.0;
    Serial.println("ADC voltage reading: " + String(transimp_out, 4));
    Serial.println("Measured Current: " + String((1.0 - transimp_out), 4) + " mA");

    Serial.println();
    for (int gate = 0; gate < 6; gate++) {
      
      int voltage_binary = i*50*(gate+1);
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
