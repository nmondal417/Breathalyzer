#include <SPI.h>

//use ESP32 VSPI pins
#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

//ADC and DAC pins
const int dacSelectPin = 5;    //slave select for DAC
const int adcSelectPin = 2;    //slave select for ADC
const int ldac = 4;            //pulse LDAC low to update DAC outputs all at once
const int vgs_dac_channels [6] = {0, 1, 2, 3, 5, 6};   //address bits for gates 1-6 of the DAC
const int vds_dac_channel = 4;

//sweep parameters
const float VGS_SWEEP_RATE = 500;  //VGS sweep rate in ms
const float VDS_HOLD = 10000;       //VDS_hold at beginning of sweep
const int numReadings = 100;  //number of readings taken for each FET in the array
const float VMID = 1;               //offset voltage for VGS and VDS

const float VGS_START = -0.5;       
const float VGS_STOP = 0.5;
const float VGS_INC = 5e-3;
float vgs;
int vgs_bin;

const float VDS_START = 900e-3;
const float VDS_STOP = 900e-3;
const float VDS_INC = 5e-3;
float vds;
int vds_bin;


//source and drain mux pins
int drain_sel [5] = {12, 13, 14, 15, 16};   //pin 12 is the LSB, pin 16 is the MSB
int source_sel [5] = {21, 22, 25, 26, 27};  //pin 21 is the LSB, pin 27 is the MSB
const int drain_en = 17; //must be brought low to enable drain demultiplexer
const int source_en = 32; //must be brought low to enable source multiplexers

const bool big_chip = 0;   //true if large chip is being used (6 gates, 32 source/32 drain)

String message;    //Serial print message

void setup() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  
  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)

  pinMode(ldac, OUTPUT);
  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  pinMode(drain_en, OUTPUT);
  pinMode(source_en, OUTPUT);
  for (int i = 0; i < 5; i++) {
    pinMode(drain_sel[i], OUTPUT);
    pinMode(source_sel[i], OUTPUT);
  }
  
  digitalWrite(ldac, HIGH);
  digitalWrite(dacSelectPin, HIGH);
  digitalWrite(adcSelectPin, HIGH);
  digitalWrite(drain_en, LOW);  //enable the drain demultiplexer
  digitalWrite(source_en, LOW); //enable the source multiplexer
  for (int i = 0; i < 5; i++) {
    digitalWrite(drain_sel[i], LOW);
    digitalWrite(source_sel[i], LOW);
  }
  
  setupDac();
}

void loop() {
  
  delay(1000);
  int i = 0;
  int j = 0;
  for(vgs = VGS_START; vgs <= VGS_STOP+0.5*VGS_INC; vgs = vgs + VGS_INC) {i++;} // determining data array size for exporting
  for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {j++;}
  Serial.println("256," + String(i) + "," + String(j));
  
  for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {    // added 0.5*VDS_INC just in case VDS is not exactly precise and the loop terminates early
    
    vds_bin = vbin(vds);                                                    // convert desired voltage into appropriate binary value
    writeDac(vds_dac_channel, vds_bin);
    ldac_pulse();
    
    for(vgs = VGS_START; vgs <= VGS_STOP+0.5*VGS_INC; vgs = vgs + VGS_INC) {  // added 0.5*VGS_INC just in case VGS is not exactly precise and the loop terminates early
      
      vgs_bin = vbin(vgs);                                       // convert desired voltage into appropriate binary value
      writeDac(vgs_dac_channels[0], vgs_bin);                    // write DAC value to 1 of the 6 gates
      ldac_pulse();
      if(vgs == VGS_START) {delay(VDS_HOLD);}
      else {delay(VGS_SWEEP_RATE);}                             // delay to allow EDL to reach steady state
      
      message = sweep(vds,vgs);
      Serial.print(message); 
    }
  }
  
  Serial.println("END");
  
}

void setupDac() {
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(dacSelectPin, LOW);
  vspi->transfer16(0b1 << 15);   //set up DAC with gain of 1, unbuffered on all channels
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
  vspi->transfer16(((0b111 & chan) << 4) + val);   //bits 14 through 12 for channel select, lower 12 bits for DAC data
  digitalWrite(dacSelectPin, HIGH);
  vspi->endTransaction();
}

int readAdc() {
  word adc_stream;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(adcSelectPin, LOW);
  adc_stream = vspi->transfer16(0);   //read ADC data
  digitalWrite(adcSelectPin, HIGH);
  vspi->endTransaction();

  return (((0b00) << 14) && adc_stream) >> 1;   //ignore the first two bits and the last bit of the adc data
}

void ldac_pulse() {
  digitalWrite(ldac, LOW);
  digitalWrite(ldac, HIGH);
}

int vbin(float desired_voltage) {
  int binary_value;
  desired_voltage = desired_voltage + VMID;
  binary_value = int(desired_voltage / (2*VMID) * 4096 );
  return binary_value;
}

//gate_or_source: 0 if drain, 1 if source
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

String sweep(float vds, float vgs) {
  unsigned long t1;
  int vmeas_average = 0;
  int vmeas = 0;
  
  t1 = millis();
  String message = "";
  message += String(t1);
  message += ",";
  message += String(vds,4);                        // gives VDS with 4 decimals of precision
  message += ",";
  message += String(vgs,4);                        // gives VGS with 4 decimals of precision
  message += ",";

  int multiplexer_max = big_chip ? 32 : 16;        //if big chip is being used, there are 32 drains/sources; small chip only has 16
  
  for (int vds_select = 0; vds_select < multiplexer_max; vds_select++) {   
    for (int vmeas_select = 0; vmeas_select < multiplexer_max; vmeas_select++) {      
      multiplexer(vds_select, 0);
      multiplexer(vmeas_select, 1);
      
      vmeas_average = 0;
      
      if(vds_select == 0 && vmeas_select == 0) {readAdc();}               // if it is the first read in a sweep, do a dummy read to give ADC caps some time to settle to proper value
      
      for (int i = 0; i < numReadings; i++) { 
        vmeas = readAdc();
        vmeas_average = vmeas_average + vmeas; 
      }
      vmeas_average = (vmeas_average - VMID) / numReadings;
      message += String(vmeas_average);                           
      message += ",";
    }
  }
  message = message.substring(0, message.length() - 1);  // removes the last comma so that when comma delimited we have an array with 256 elements and not 257
  message += "\n";
  return message;
}
