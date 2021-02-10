/*
 * sd sound
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10 
 *
 * Created: 10/02/2021
 *  Author: moty22.co.uk
 */
  
#include <SPI.h>
#include <SD.h>

const int CS = 10;          // SD CS connected to digital pin PD4
const int mosi = 11;
const int clk = 13;
const int miso = 12;          // Slave Select - miso 
const int RecLED = 8;          // led connected to digital pin PB0
const int ErrorLED = 9;          // error led connected to digital pin PB1
const int Stop = A2;     //stop pushbutton C2
const int Play = A3;   //play PB C3
const int Rec = A4;      //record PB C4
const int Pause = A5;    //pause PB C5
const int mic = A0;     //microphone
const int audio = 6;     //output D6
const int type = 2;
int stopPB=1;         // stop pushbutton status
int playPB=1;         // play pushbutton status
int recPB=1;         // rec pushbutton status
int pausePB=1;         // pause pushbutton status

unsigned long loc;  //pause location
bool sdhc; //sd/sdhc


void setup() {
  // put your setup code here, to run once:
pinMode(CS, OUTPUT);
pinMode(mosi, OUTPUT);
pinMode(clk, OUTPUT);
pinMode(RecLED, OUTPUT);
pinMode(ErrorLED, OUTPUT);
pinMode(Stop, INPUT_PULLUP);
pinMode(Play, INPUT_PULLUP);
pinMode(Rec, INPUT_PULLUP);
pinMode(Pause, INPUT_PULLUP);
pinMode(miso, INPUT);          // set SPI to master
//pinMode(audio, OUTPUT);
pinMode(mic, INPUT);
pinMode(type, INPUT);

SD.begin(4);

  //PWM Timer0
OCR0A = 64;
TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);  //output in phase, fast PWM mode
TCCR0B = _BV(CS00); // 16MHz/256 = 64KHz
pinMode(audio, OUTPUT);

digitalWrite(RecLED, LOW); 
digitalWrite(ErrorLED, LOW);

if(digitalRead(type)){sdhc=true;}else{sdhc=false;}
if(sdhc){loc = 10;}else{loc = 5120;}    // SDHC address is the number of sectors
}


void loop()
{

        //look at the pushbuttons 
    pausePB=digitalRead(Pause);  
    recPB=digitalRead(Rec);  
    if(recPB==LOW) WriteSD();
    playPB=digitalRead(Play);
    if(playPB==LOW) ReadSD();
    stopPB=digitalRead(Stop);
    if(stopPB==LOW) {
    if(sdhc){loc = 10;}else{loc = 5120;}
    }

}

unsigned char spi(unsigned char data)   // send character over spi
{
  SPDR = data;  // Start transmission
  while(!(SPSR & _BV(SPIF)));   // Wait for transmission to complete
  return SPDR;    // received byte

}
      //assemble a 32 bits command
char Command(unsigned char frame1, unsigned long adrs, unsigned char frame2 )
{
  unsigned char i, res;
  spi(0xFF);
  spi((frame1 | 0x40) & 0x7F);  //first 2 bits are 01
  spi((adrs & 0xFF000000) >> 24);   //first of the 4 bytes address
  spi((adrs & 0x00FF0000) >> 16);
  spi((adrs & 0x0000FF00) >> 8);
  spi(adrs & 0x000000FF);
  spi(frame2 | 1);        //CRC and last bit 1

  for(i=0;i<10;i++) // wait for received character
  {
    res = spi(0xFF);
    if(res != 0xFF)break;
  }
  return res;
}

  void WriteSD(void)
  {
    unsigned int r,i;
    unsigned char hbyte,lbyte;
    digitalWrite(CS, LOW);
    ADCSRA |=_BV(ADEN); //ADC enabled
    digitalWrite(RecLED, HIGH);
    
    r = Command(25,loc,0xFF); //multi sector write
    if(r != 0)
    {
      digitalWrite(ErrorLED, HIGH);
      ADCSRA &=~(_BV(ADEN));  //ADC disabled
      digitalWrite(RecLED, LOW);
    }
    spi(0xFF);
    spi(0xFF);
    spi(0xFF);

    while(stopPB==HIGH && pausePB==HIGH)
    {
      spi(0xFC);  //multi sector token byte
      for(i=0;i<512;i++)    //stream 512 bytes from ADC to build a sector
      {
        lbyte=(unsigned char)analogRead(mic);
        spi(lbyte);    //record analogue byte
        OCR0A=lbyte;         //comment this line to disable play while recording
      }
      spi(0xFF);  // CRC
      spi(0xFF);  // CRC
      
      if((r=spi(0xFF) & 0x0F) == 0x05){ //test if data accepted    
        for(i=10000;i>0;i--){
          if(r=spi(0xFF))  break;
        }
      }
      else{
        digitalWrite(ErrorLED, HIGH);
      }
      while(spi(0xFF) != 0xFF); // while card busy to write sector
      if(sdhc){loc += 1;}else{loc += 512;}  //mark location address for return after pause
      stopPB=digitalRead(Stop);
      pausePB=digitalRead(Pause);
    }
    spi(0xFD);  //'stop transfer' token byte is needed to stop multi-sector
    
    spi(0xFF);
    spi(0xFF);
    while(spi(0xFF) != 0xFF); // while busy
   
    digitalWrite(CS, HIGH);
    ADCSRA &=~(_BV(ADEN));    //ADC disabled
    digitalWrite(RecLED, LOW);
  }

  void ReadSD(void)
  {
    unsigned int i,r;
    unsigned char read_data;
    digitalWrite(CS, LOW);
    r = Command(18,loc,0xFF); //read multi-sector
    if(r != 0)digitalWrite(ErrorLED, HIGH);     //if command failed

    while(stopPB==HIGH && pausePB==HIGH)
    {
      while(spi(0xFF) != 0xFE); // wait for first byte
    for(i=0;i<512;i++){
        OCR0A=spi(0xFF);    //write byte to PWM
        delayMicroseconds(65);
    }
      spi(0xFF);  //discard of CRC
      spi(0xFF);
      
      if(sdhc){loc += 1;}else{loc += 512;}  //mark location address for return after pause

      stopPB=digitalRead(Stop);
      pausePB=digitalRead(Pause);
     }
    
    Command(12,0x00,0xFF);  //stop transmit
    spi(0xFF);
    spi(0xFF);
    digitalWrite(CS, HIGH);
  }
