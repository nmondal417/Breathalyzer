//test to see how long digitalWrites take on ESP32
//answer is that it is much faster than Arduinos

unsigned long NUM_CYCLES = 1000000;

void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);

  Serial.begin(115200);
  delay(100);

  
}

void loop() {

  long starting_time = millis();  /*
  for (int i = 0; i < NUM_CYCLES; i++) {
    REG_WRITE(GPIO_OUT_W1TS_REG, BIT2);
    REG_WRITE(GPIO_OUT_WITC_REG, BIT2);
  }
  Serial.println("Direct Port: " + String(millis() - starting_time));

  delay(1000);*/
  
  starting_time = millis();
  for (int i = 0; i < NUM_CYCLES; i++) {
    digitalWrite(2, LOW);
    digitalWrite(2, HIGH);
  }
  Serial.println("Digital Write: " + String(millis() - starting_time)); 

}
