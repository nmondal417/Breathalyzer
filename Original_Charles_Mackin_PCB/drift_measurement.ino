// Verified that Gain is ~10,000 V/A
// Verified that current increases roughly linearly with VDS for metal test chip
// Important to disable mux before recording VDS values. ADC produces more accurate data when not loaded by the mux and FET array
// Important to dummy sample ADCs before each FET array sweep. This allows the ADC sampling caps to reach stable values before actual ADC data is recorded. Otherwise, the first FET sampled (row=0,col=0) will be an outlier
// Test chip resistance is not perfectly constant because VDS applied is not precisely VDS transmitted (there is some error, which causes sloping artifact in RDS curves): NEED TO MAKE VDS (AND VGS) MORE ACCURATE


// CALIBRATIION PROCEDURE
// WRITE 0 to DAC output and record voltage with respect to ground as VMIN
// WRITE 4095 to DAC output and record voltage with respect to ground as VMAX
// SLOPE = ( VMAX - VMIN ) / 4095
// Binary value to generate desired voltage with respect to VMID = int { (1/SLOPE)*(desired_voltage+VMID) + VMIN/SLOPE }
// NOW DAC SHOULD OUTPUT PRECISELY THE DESIRED VOLTAGE

// CALIBRATION PROCEDURE (MAKING SURE ADC READS APPLIED DAC VALUES VERY ACCURATELY)
// MEASURE ARDUINO MC POWER SUPPLY VERY ACCURATELY
// ADC VOLTAGE MEASURED = (ARDUINO_SUPPLY_VOLTAGE/4095)*ADC_BITS
// MEASUREMENT SHOULD NOW BE ACCURATE TO WITHIN A 1mV OR SO


// IMPORTANT: Peripheral Identifiers on page 38 (PIOC, DACC, etc)


float VDS = 300e-3;
float VGS = 0;
float VMID = 1.499;

int numReadings = 100;

float refresh_delay = 100; // array refresh delay in ms, if you want to read as fast as possible set to zero

int VDS_select;
int VOUT_select;
int VOUT_digital;
int VOUT_analog;
int VDS_bin;
int VGS_bin;
int i = 0;
int bytesSent;

unsigned long t1;
unsigned long t2;
unsigned long t3;

String message;


/*
// pin mapping
pin   0b
 3 -> 28
 4 -> 26
 5 -> 25
 6 -> 24
 7 -> 23
 8 -> 22
 9 -> 21
10 -> 29

VDS
44 -> 19
45 -> 18
46 -> 17
47 -> 16

VOUT
48 -> 15
49 -> 14
50 -> 13
51 -> 12

MUX ENABLES ARE PINS 40 and 41

53,0,13 -> 20 not mapped

*/

void setup() {
  // put your setup code here, to run once:
  // See page 18 for Microcontroller Pinout (PA=PIOA, PB=DACC, PC=PIOC, PD=PIOD)
  
  // Set up Registers for VDS and VOUT Select Line Operation
  // See page 621, 631 for Register Information
  PIOC -> PIO_PER    = 0b11111111111111111111111111111111;  // 1:  PIO Enable Register
  PIOC -> PIO_PDR    = 0b00000000000000000000000000000000;  // 2:  PIO Disable Register
  //PIOC -> PIO_PSR    = 0b00000000000000000000000000000000;  // 3:  PIO Status Register
  PIOC -> PIO_OER    = 0b11111111111111111111111111111111;  // 4:  Output Enable Register
  PIOC -> PIO_ODR    = 0b00000000000000000000000000000000;  // 5:  Output Disable Register
  //PIOC -> PIO_OSR    = 0b00000000000000000000000000000000;  // 6:  Output Status Register
  PIOC -> PIO_IFER   = 0b00000000000000000000000000000000;  // 7:  Glitch Input Filter Enable Register
  PIOC -> PIO_IFDR   = 0b11111111111111111111111111111111;  // 8:  Glitch Input Filter Disable Register
  //PIOC -> PIO_IFSR   = 0b00000000000000000000000000000000;  // 9:  Glitch Input Filter Status Register
  //PIOC -> PIO_SODR   = 0b00000000000000000000000000000000;  // 10: Set Output Data Register
  PIOC -> PIO_CODR   = 0b11111111111111111111111111111111;  // 11: Clear Output Data Register
  //PIOC -> PIO_ODSR   = 0b00000000000000000000000000000000;  // 12: Output Data Status Register
  PIOC -> PIO_PDSR   = 0b00000000000000000000000000000000;  // 13: Pin Data Status Register
  PIOC -> PIO_IER    = 0b11111111111111111111111111111111;  // 14: Interrupt Enable Register
  PIOC -> PIO_IDR    = 0b00000000000000000000000000000000;  // 15: Interrupt Disable Register
  PIOC -> PIO_IMR    = 0b00000000000000000000000000000000;  // 16: Interrupt Mask Register
  //PIOC -> PIO_ISR    = 0b00000000000000000000000000000000;  // 17: Interrupt Status Register
  PIOC -> PIO_MDER   = 0b00000000000000000000000000000000;  // 18: Multi-driver Enable Register
  PIOC -> PIO_MDDR   = 0b11111111111111111111111111111111;  // 19: Multi-driver Disable Register
  //PIOC -> PIO_MDSR   = 0b00000000000000000000000000000000;  // 20: Multi-driver Status Register
  PIOC -> PIO_PUDR   = 0b11111111111111111111111111111111;  // 21: Pull-up Disable Register
  PIOC -> PIO_PUER   = 0b00000000000000000000000000000000;  // 22: Pull-up Enable Register
  //PIOC -> PIO_PUSR   = 0b00000000000000000000000000000000;  // 23: Pad Pull-up Status Register
  PIOC -> PIO_ABSR   = 0b00000000000000000000000000000000;  // 24: Peripheral AB Select Register
  PIOC -> PIO_SCIFSR = 0b00000000000000000000000000000000;  // 25: System Clock Glitch Input Filter Select Register
  PIOC -> PIO_DIFSR  = 0b00000000000000000000000000000000;  // 26: Debouncing Input Filter Select Register
  //PIOC -> PIO_IFDGSR = 0b00000000000000000000000000000000;  // 27: Glitch or Debouncing Input Filter Clock Selection Status Register
  PIOC -> PIO_SCDR   = 0b00000000000000000000000000000000;  // 28: Slow Clock Divider Debouncing Register
  PIOC -> PIO_OWER   = 0b11111111111111111111111111111111;  // 29: Output Write Enable
  PIOC -> PIO_OWDR   = 0b00000000000000000000000000000000;  // 30: Output Write Disable
  //PIOC -> PIO_OWSR   = 0b00000000000000000000000000000000;  // 31: Output Write Status Register
  PIOC -> PIO_AIMER  = 0b00000000000000000000000000000000;  // 32: Additional Interrupt Modes Enable Register
  PIOC -> PIO_AIMDR  = 0b11111111111111111111111111111111;  // 33: Additional Interrupt Modes Disables Register
  PIOC -> PIO_AIMMR  = 0b00000000000000000000000000000000;  // 34: Additional Interrupt Modes Mask Register
  PIOC -> PIO_ESR    = 0b00000000000000000000000000000000;  // 35: Edge Select Register
  PIOC -> PIO_LSR    = 0b00000000000000000000000000000000;  // 36: Level Select Register
  //PIOC -> PIO_ELSR   = 0b00000000000000000000000000000000;  // 37: Edge/Level Status Register
  PIOC -> PIO_FELLSR = 0b00000000000000000000000000000000;  // 38: Falling Edge/Low Level Select Register
  PIOC -> PIO_REHLSR = 0b00000000000000000000000000000000;  // 39: Rising Edge/ High Level Select Register
  PIOC -> PIO_FRLHSR = 0b00000000000000000000000000000000;  // 40: Fall/Rise - Low/High Status Register
  //PIOC -> PIO_LOCKSR = 0b00000000000000000000000000000000;  // 41: Lock Status
  PIOC -> PIO_WPMR   = 0b00000000000000000000000000000000;  // 42: Write Protect Mode Register 
  //PIOC -> PIO_WPSR   = 0b00000000000000000000000000000000;  // 43: Write Protect Status Register
  
  analogReadResolution(12);
  analogWriteResolution(12);
  VDS_bin = VBIN(VDS);
  VGS_bin = VBIN(VGS);
  analogWrite(DAC0, VDS_bin);      // values can range from 0-4095: 0->0.53V, 4095->2.66V
  analogWrite(DAC1, VGS_bin);      // values can range from 0-4095: 0->0.53V, 4095->2.66V
  
  /*
  // Set up DAC
  // See page 1356, 1358, 1362, 1412, 1411
  // DAC uses master clock (4 MHz by default) divided by 2 and takes 25 cycles to produce the desired output
  DACC -> DACC_WPMR  = 0b01000100010000010100001100000000;  // 1: Write Protect Mode Register: Disables Write Protect
  //DACC -> DACC_WPSR = 0b00000000000000000000000000000000;  // 23: Write Protect Status Register
  DACC -> DACC_CR    = 0b00000000000000000000000000000000;  // 3: Control Register
  DACC -> DACC_MR    = 0b00010111001000000000000000010000;  // 4: Mode Register: EMPTY=00, STARTUP=010111, EMPTY=00, MAXS=1, TAG=0, EMPTY=00, USER_SEL=00, REFRESH=00000000, EMPTY=0, FASTWKUP=0, SLEEP=0, WORD=1, TRGSEL=000, TRGEN=0 
  DACC -> DACC_CHER  = 0b00000000000000000000000000000001;  // 5: Channel Enable Register: Enables Channel 0
  DACC -> DACC_CHDR  = 0b00000000000000000000000000000000;  // 6: Channel Disable Register
  //DACC -> DACC_CHSR = 0b00000000000000000000000000000000;  // 7: Channel Status Register
  DACC -> DACC_CDR   = 0b00000000000000000000000000000000;  // 8: Conversion Data Register:  (1/6)*VADVREF - (5/6)*VADVREF or about 0.5V - 2.75V
  DACC -> DACC_IER   = 0b00000000000000000000000000000000;  // 9: Interrupt Enable Register
  DACC -> DACC_IDR   = 0b00000000000000000000000000001111;  // 10: Interrupt Disable Register
  DACC -> DACC_IMR   = 0b00000000000000000000000000000000;  // 11: Interrupt Mask Register
  //DACC -> DACC_ISR = 0b00000000000000000000000000000000;   // 12: Interrupt Status Register
  DACC -> DACC_ACR   = 0b00000000000000000000000000000000;  // 13: Analog Current Register
  */
  
  Serial.begin(115200);                // Arduino Due Programming Port: Slow transmission, but need to open this line of communication because it auto resets arduino, which also resets the native port usb communication
  while (!Serial) {}                   // wait for serial ports to connect. Needed for Arduino DUE only
  
  //SerialUSB.begin(115200);           // Arduino Due Native Port: Fast transmission, baudrate for Native Port is actually ignored (transmits data as fast as both sides can go)
  //while (!SerialUSB) {}
}

void loop() {
  delay(refresh_delay);
  message = sweep();
  bytesSent = Serial.print(message);
}


String sweep() {
  float vout_average = 0;
  float vgs_average = 0;
  float vds_average = 0;
  float vmid_average = 0;
  float vmid = 0;
  float vout = 0;
  int i = 0;
  int VDS_select;
  int VOUT_select;
  
  /*PIOC -> PIO_ODSR = 0b00000000000000000000000000000000;    // set mux enable off to ensure more precise ADC readings
  for (i = 0; i < numReadings; i++) {
    analogRead(A3);                                         // add some dummy reads to allow value to stabilize. produces more accurate VDS and VGS values
    analogRead(A3);
    vmid_average = vmid_average + analogRead(A3);
    analogRead(A2);
    analogRead(A2);
    vds_average = vds_average + analogRead(A2);
    analogRead(A1);
    analogRead(A1);
    vgs_average = vgs_average + analogRead(A1); 
  }
  vmid_average = vmid_average/numReadings;
  vds_average = vds_average/numReadings;
  vgs_average = vgs_average/numReadings;
  */
  
  t1 = millis();
  String message = "";
  message += String(t1);
  message += ",";
  //message += String(vds_average - vmid_average);
  message += String(VDS,4);                          // gives VDS with 4 decimals of precision
  message += ",";
  //message += String(vgs_average - vmid_average);
  message += String(VGS,4);                          // gives VGS with 4 decimals of precision
  message += ",";
  message += String(vmid_average);
  message += ",";
  
  for (VDS_select = 0; VDS_select < 16; VDS_select++) {   
    for (VOUT_select = 0; VOUT_select < 16; VOUT_select++) {      
      PIOC -> PIO_ODSR = (VDS_select << 16) | (VOUT_select << 12) | (768);                // write VDS select bits and VOUT select bits, 768 sets pins 40 and 41 high
      vout_average = 0;
      vmid_average = 0;
      
      if(VDS_select == 0 && VOUT_select == 0) {analogRead(A3); analogRead(A0);}               // if it is the first read in a sweep, do a dummy read to give ADC caps some time to settle to proper value
       
      for (i = 0; i < numReadings; i++) { 
        vmid = analogRead(A3);
        vout = analogRead(A0);
        vout_average = vout_average + vout; 
        vmid_average = vmid_average + vmid; 
      }
      //Serial.println(vmid_average / numReadings);                                           // can use to verify that VMID stil produces about 1883 for this
      vout_average = (vout_average - vmid_average) / numReadings;
      message += String(vout_average);                                  // now read the input on the analog pin 0 (ADC)
      message += ",";
    }
  }
  message = message.substring(0, message.length() - 1);  // removes the last comma so that when comma delimited we have an array with 256 elements and not 257
  message += "\n";
  return message;
}


// Derived Conversion
int VBIN(float desired_voltage) {
  int binary_value;
  desired_voltage = desired_voltage + VMID;
  binary_value = int( 1881.9798752*desired_voltage - 1021.91507223 );
  return binary_value;
}

/*
Calibration Data
DAC 0 --> 0.543 V
DAC 4095 --> 2.7189 V
SLOPE = 0.00053135531
VBIN = int { (1/SLOPE)*(desired_voltage+VMID) + VMIN/SLOPE }
1/SLOPE = 1881.9798752
VMIN/SLOPE = 1021.91507223
VMID = 1.499 V
*/

