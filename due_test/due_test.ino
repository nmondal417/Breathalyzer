//test to see the speed of different methods of writing to Arduino Due pins

int NUM_CYCLES = 1000000;

void setup() {
  // put your setup code here, to run once:
  pinMode(51, OUTPUT);


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
  //PIOC -> PIO_WPSR   = 0b00000000000000000000000000000000;  // 43: Write Protect Status Register */
  
  analogWriteResolution(12);

  Serial.begin(115200);
  while(!Serial) {}
}

void loop() {

  long starting_time = millis();
  for (int i = 0; i < NUM_CYCLES; i++) {
    PIOC -> PIO_ODSR = 1<< 12; 
    PIOC -> PIO_ODSR = 0 << 12;
  }
  Serial.println("Direct Port: " + String(millis() - starting_time));

   
  starting_time = millis();
  for (int i = 0; i < NUM_CYCLES; i++) {
    digitalWrite(51, LOW);
    digitalWrite(51, HIGH);
  }
  Serial.println("Digital Write: " + String(millis() - starting_time)); 
  //analogWrite(DAC0, 4095);

}
