#include <SPI.h>

/*
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_CS   15
SPIClass SPI1(HSPI);
SPIClass * hspi = NULL;
*/
#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK
//#define VSPI_SS     SS
#define LDAC        22
#define A_INPUT_1   15
#define B_INPUT_1   34
#define A_INPUT_2   4
#define CS1         5
#define CS2         2

#if CONFIG_IDF_TARGET_ESP32S2
#define VSPI FSPI
#endif

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;

static const unsigned long spiClk = 4000000; 

byte channel_sel;  //0 for channel A, 1 for channel B
byte gain;   //0 for 2x gain, 1 for 1x gain
byte sdhn;   //0 for inactive, 1 for active
word dac1_data; //12-bit input data to DAC
word dac2_data;
word write_msg; //16-bit message sent to DAC
float channel_A;
float channel_B;
float channel_A_2;

void setup() {
  vspi = new SPIClass(VSPI);
  
  vspi->begin();
  //pinMode(vspi->pinSS(), OUTPUT); //VSPI SS
  pinMode(CS2, OUTPUT);
  digitalWrite(CS2, HIGH);
  pinMode(CS1, OUTPUT);
  digitalWrite(CS1, HIGH);

  channel_sel = 0b0;
  gain = 0b1;
  sdhn = 0b1;
  dac1_data = 0b000000000;
  dac2_data = 0b000000000;
  write_msg = 0b0000000000000000;

  pinMode(B_INPUT_1, INPUT);
  pinMode(A_INPUT_1, INPUT);
  pinMode(LDAC, OUTPUT);
  digitalWrite(LDAC, LOW);

  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)
  
  //SPI1.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  //pinMode(SPI_CS, OUTPUT);
  //SPI1.setClockDivider(15, 21);
  //SPI1.setBitOrder(15, MSBFIRST);
  //SPI1.setDataMode(15, SPI_MODE0);
  /*
  hspi = new SPIClass(HSPI);
  hspi->begin();
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_CS); //SCLK, MISO, MOSI, SS
  pinMode(HSPI_CS, OUTPUT); //HSPI SS*/
}

void loop() {
  digitalWrite(LDAC, HIGH);
  channel_sel = 0b0;
  write_msg = (channel_sel << 15) + (gain << 13) + (sdhn << 12) + dac1_data;
  
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  //digitalWrite(vspi->pinSS(), LOW); //pull SS low to prep other end for transfer
  digitalWrite(CS1, LOW);
  vspi->transfer16(write_msg);
  //digitalWrite(vspi->pinSS(), HIGH);
  digitalWrite(CS1, HIGH);
  vspi->endTransaction();

  channel_A = 3.3*(analogRead(A_INPUT_1)/4095.0);
  //channel_B = 3.3*(analogRead(B_INPUT_1)/4095.0);
  channel_A_2 = 3.3*(analogRead(A_INPUT_2)/4095.0);

  Serial.print("Channel A #1: ");
  Serial.print(channel_A, 2);
  //Serial.print("Channel_B: ");
  //Serial.print(channel_B, 2);
  Serial.print("Channel A #2: ");
  Serial.print(channel_A_2, 2);
  Serial.println("-------------");

  
  channel_sel = 0b0;
  write_msg = (channel_sel << 15) + (gain << 13) + (sdhn << 12) + dac2_data;
 
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  //digitalWrite(vspi->pinSS(), LOW); //pull ss high to signify end of data transfer
  digitalWrite(CS2, LOW);
  vspi->transfer16(write_msg);
  //digitalWrite(vspi->pinSS(), HIGH);
  digitalWrite(CS2, HIGH);
  vspi->endTransaction();
  


  digitalWrite(LDAC, LOW);

   channel_A = 3.3*(analogRead(A_INPUT_1)/4095.0);
  //channel_B = 3.3*(analogRead(B_INPUT_1)/4095.0);
  channel_A_2 = 3.3*(analogRead(A_INPUT_2)/4095.0);

  Serial.print("Channel A #1: ");
  Serial.print(channel_A, 2);
  //Serial.print("Channel_B: ");
  //Serial.print(channel_B, 2);
  Serial.print("Channel A #2: ");
  Serial.print(channel_A_2, 2);
  Serial.println("-------------");

  dac1_data += (1 << 10);
  dac1_data &= ~(0b1111 << 12);

  dac2_data += (1 << 9);
  dac2_data &= ~(0b1111 << 12);
  delay(2000);
}
