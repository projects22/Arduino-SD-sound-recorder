/*
  SD wave
  by moty22.co.uk
  
   SD card attached to SPI bus as follows:
 * MOSI - pin 11
 * MISO - pin 12
 * CLK - pin 13
 * CS - pin 10
*/

#include <SPI.h>
#include <SD.h>

File f1;

void setup() {
  
  pinMode(A3, INPUT_PULLUP);  //play
  pinMode(A2, INPUT_PULLUP);  //stop
  pinMode(A4, INPUT_PULLUP);  //record
  pinMode(8, OUTPUT);  //rec LED
  Serial.begin(9600);
  while (!Serial) {}

  SD.begin(10);

    //PWM Timer0 audio PWM
  OCR0A = 64;
  TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);  //output in phase, fast PWM mode
  TCCR0B = _BV(CS00); // 16MHz/256 = 64KHz
  pinMode(6, OUTPUT);

}

void loop() {
  if(!digitalRead(A3))play();
  if(!digitalRead(A4))record();
}

void play(){
    f1 = SD.open("w1.wav");

    while (f1.available()) {
      OCR0A=f1.read();
      delayMicroseconds(70);
      if(!digitalRead(A2))break; 
    }
    f1.close();
}

void record(){
  unsigned char Lb; 
  
  digitalWrite(8, HIGH);  //rec LED
  f1 = SD.open("w1.wav", FILE_WRITE);
  while(digitalRead(A2)){
    Lb=(unsigned char)analogRead(A0);
    f1.write(Lb);
  }
  f1.close();
  digitalWrite(8, LOW);  //rec LED
}
