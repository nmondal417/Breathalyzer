#include <Arduino.h>
#include <SPI.h>

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

const int dacSelectPin = 16;
const int adcSelectPin = 2;

const int vgs_dac_channels [6] = {1, 0, 2, 4, 6, 5};   //DAC address bits for gate voltages 1-6
const int vds_dac_channel = 3; //channel for drain voltage
const bool amp = false;     //true for x10 amplification, false for x1

//CHANGE THIS TO MATCH RESISTOR BEING USED
const float test_resistor = 1000;  //resistor used to test transimpedance amp; placed between drain and source voltages

//source and drain mux pins
int drain_sel [5] = {12, 0, 14, 15, 5};   //pin 12 is the LSB, pin 5 is the MSB
int source_sel [5] = {21, 4, 25, 26, 27};  //pin 21 is the LSB, pin 27 is the MSB

const int chip_en = 33;  //must be brought low to enable the multiplexers and the gate switch
const int gate_amp = 32;   //bring LOW for x1 amplification, bring HIGH for x10 amplification
const int gate_shdn = 22;  //bring HIGH to enable gate voltage converter

int powers_of_two [5] = {1, 2, 4, 8, 16};

void setupDac();
void writeDac(unsigned int chan, unsigned int val);
float readAdc();
void multiplexer(int address, bool drain_or_source);

void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  delay(500); //wait a bit (100 ms)

  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(adcSelectPin, HIGH);

  
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

  digitalWrite(gate_shdn, LOW);
  delay(4000);

  //IMPORTANT: set amplification and turn on gate switch before turning on the gate voltage converter,
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

    int vds_bin = 400 * i;  //sweep Vds from 0V to 2V
    float vds = vds_bin/4096.0*2.0;
    
    writeDac(vds_dac_channel, vds_bin);  

    Serial.println();
    Serial.println("DAC output: " + String(vds));
    Serial.println("Expected Current: " + String((vds - 1.0)/test_resistor*1000, 4) + " mA");
    
    //CHANGE THIS TO TEST DIFFERENT MUTLIPLEXER CHANNELS
    multiplexer(0, 0);   //drain multiplexer channel
    multiplexer(31, 1);  //source multiplexer channel

    float transimp_out = readAdc()/4096.0 * 2.0;
    Serial.println("ADC voltage reading: " + String(transimp_out, 4));
    Serial.println("Measured Current: " + String((1.0 - transimp_out), 4) + " mA");

    Serial.println();

    //gate voltage sweep
    for (int gate = 0; gate < 6; gate++) {
      
      int voltage_binary = i*50*(gate+1);  //run a different voltage sweep for each gate, just to test stuff out
      
      float true_voltage = amp ? ((voltage_binary/4096.0*2.0 - 1)*10.1) + 1: //amplify voltage if needed
                                   voltage_binary/4096.0*2.0;
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