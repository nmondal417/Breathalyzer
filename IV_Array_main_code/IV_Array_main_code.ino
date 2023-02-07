#include <SPI.h>
#include <string.h>

//use ESP32 VSPI pins
#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 800000;  //SPI clock frequency

//ADC and DAC pins
const int dacSelectPin = 5;    //slave select for DAC
const int adcSelectPin = 2;    //slave select for ADC
const int ldac = 4;            //pulse LDAC low to update DAC outputs all at once
const int vgs_dac_channels [6] = {0, 1, 2, 3, 5, 6};   //DAC channels for gates 1-6
const int vds_dac_channel = 4;   //DAC channel for drain

//gate voltage amplification
const float amps [3] = {1, 10.1, 20.1};             //amplification choices for each gate
const int desired_amps [6] = {0, 0, 0, 0, 0, 0};    //0 for x1, 1 for x10, 2 for x20
//NOTE: changing the values in desired_amps will not actually change the gate amplification, only the print output 
//is changed. The jumpers on the board determine the amplification for each gate.

//sweep parameters
float VGS_SWEEP_RATE = 0;  //VGS sweep rate in ms
float VDS_HOLD = 0;       //VDS_hold at beginning of sweep
int numReadings = 10;  //number of readings taken for each FET in the array
int numDummy = 10;
float VMID = 1;               //offset voltage for VGS and VDS

//Vgs parameters for gates 1 through 6
float VGS_START [6] = {-0.5, -0.5, -0.5, -0.5, -0.5, -0.5};       
float VGS_STOP [6] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
float VGS_INC [6] = {10e-3, 10e-3, 10e-3, 10e-3, 10e-3, 10e-3};
float VGS_OFFSETS [6] = {0, 0, 0, 0, 9e-3, 9e-3};  //positive offset means the DAC outputs a higher-than-expected value
float vgs [6];
int vgs_bin [6];  //VGS as a value from 0 to 4095

//Vds parameters
const float VDS_START = 900e-3;
const float VDS_STOP = 900e-3;
const float VDS_INC = 5e-3;
float VDS_OFFSET = 19e-3; //positive offset means the DAC outputs a higher-than-expected value
float vds;
int vds_bin;   //VDS as a value from 0 to 4095 (0V = 0, 2V = 4095)

//source and drain mux pins
int drain_sel [5] = {12, 13, 14, 15, 16};   //pin 12 is the LSB, pin 16 is the MSB
int source_sel [5] = {21, 22, 25, 26, 27};  //pin 21 is the LSB, pin 27 is the MSB
const int drain_en = 17; //must be brought low to enable drain demultiplexer
const int source_en = 32; //must be brought low to enable source multiplexers

const bool big_chip = 1;   //true if large chip is being used (6 gates, 32 source/32 drain)

String message;    //Serial print message

unsigned long t1;
double t2;
unsigned long t3;

int powers_of_two [5] = {1, 2, 4, 8, 16};

void setup() {
  t1 = millis();
  t3 = millis();
  vspi = new SPIClass(VSPI);  //begin SPI
  vspi->begin();
  //vspi->begin(VSPI_SCLK, VSPI_MISO, VSPI_MOSI, VSPI_SS);
  //pinMode(vspi->pinSS(), OUTPUT);
  Serial.begin(115200); //begin serial comms
  delay(1000); //wait a bit (100 ms)

  pinMode(ldac, OUTPUT);
  pinMode(dacSelectPin, OUTPUT);
  pinMode(adcSelectPin, OUTPUT);
  pinMode(drain_en, OUTPUT);
  pinMode(source_en, OUTPUT);
  for (int i = 0; i < 5; i++) {
    pinMode(drain_sel[i], OUTPUT);
    pinMode(source_sel[i], OUTPUT);
  }
  
  digitalWrite(ldac, HIGH);          //initialize DAC and ADC control pins
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
  
  int i = 0;
  int j = 0;
  for(vgs[0] = VGS_START[0]; vgs[0] <= VGS_STOP[0]+0.5*VGS_INC[0]; vgs[0] = vgs[0] + VGS_INC[0]) {i++;} // determining data array size for exporting
  for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {j++;}
  Serial.print("256," + String(i) + "," + String(j));

  //outer loop for Vds sweep, inner loop for Vgs sweep
  for(vds = VDS_START; vds <= VDS_STOP+0.5*VDS_INC; vds = vds + VDS_INC) {    // added 0.5*VDS_INC just in case VDS is not exactly precise and the loop terminates early
    
    vds_bin = v_to_bin(vds - VDS_OFFSET);  // convert desired voltage into appropriate binary value (0 to 4095)
    writeDac(vds_dac_channel, vds_bin);
    ldac_pulse();                         //update DAC output

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
      ldac_pulse();
      
      message = sweep(vds,vgs);
      Serial.print(message); 

      for (int gate = 0; gate < 6; gate++) {
        vgs[gate] += VGS_INC[gate];
      }
    }
  }
  
  Serial.println("END");
  
}

String sweep(float vds, float* vgs) {
  
  int vmeas_bin_avg = 0;
  int vmeas_bin = 0;
  
  
  String message = "";
  t1 = millis() - t1;
  message += String(t1);
  //message += ",";
  //message += String(t2);
  t1 = millis();
  message += ",";
  message += String(vds,4);                        // gives VDS with 4 decimals of precision
  message += ",";
  for (int gate = 0; gate < 6; gate++) {
    float vgs_amped = vgs[gate]*amps[desired_amps[gate]];
    message += String(vgs_amped,4);                        // gives VGS with 4 decimals of precision
    message += ",";
    break;   //only once for now
  }
  
  message += String(VMID);
  message += ",";
  
  

  int multiplexer_max = big_chip ? 32 : 16;        //if big chip is being used, there are 32 drains/sources; small chip only has 16
  
  for (int vds_select = 0; vds_select < multiplexer_max; vds_select++) {   
    for (int vmeas_select = 0; vmeas_select < multiplexer_max; vmeas_select++) {      
      multiplexer(vds_select, 0);
      multiplexer(vmeas_select, 1);
      //delay(1);
      
      vmeas_bin_avg = 0;
      
      for (int i = 0; i < numDummy; i++) {
        readAdc();    // if it is the first read in a sweep, do a dummy read to give ADC caps some time to settle to proper value
      }  
      //t2 = 0;
      //t3 = micros();
      for (int i = 0; i < numReadings; i++) { 
        vmeas_bin = readAdc();
        vmeas_bin_avg += vmeas_bin; 
      }
      //t3 = micros() - t3;
      //t2 += t3/1000.0;
      vmeas_bin_avg = vmeas_bin_avg / numReadings;
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
  vspi->transfer16(0b1010000000000001);   //set up DAC with with LDAC high (default mode, so not needed), DAC outputs will not update until LDAC is pulsed low
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

//Note: The ADC (MCP3201) operates in SPI Mode 0 (data is sampled on the rising edge of the clock)
float readAdc() {
  word adc_stream;
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(adcSelectPin, LOW);
  adc_stream = vspi->transfer16(0);   //read ADC data
  digitalWrite(adcSelectPin, HIGH);
  vspi->endTransaction();
  return (~(0b11 << 14) & adc_stream) >> 1;   //ignore the first two bits and the last bit of the adc data
}

void ldac_pulse() { //used to update DAC output
  digitalWrite(ldac, LOW);
  digitalWrite(ldac, HIGH);
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

//drain_or_source: 0 if drain, 1 if source
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

// Function to copy 'len' elements from 'src' to 'dst'
void copy(float* src, float* dst, int len) {
    memcpy(dst, src, sizeof(src[0])*len);
}
