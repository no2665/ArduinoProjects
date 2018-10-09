#include <TinyWireM.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define DS1307_ADDR   0x68

int clockPin = 3;
int dataPin = 4;
int buttonPin = 1;
unsigned int binaryTime = 0;
boolean buttonInterrupt = false;
byte timerCounter = 10;

void setup () {
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  TinyWireM.begin();
  GIMSK=(1 << PCIE); //Enable Pin Change Int 
  PCMSK=(1 << PCINT1); //Set PCINT1 to cause interrupt
}

void loop () {
  if(++timerCounter >= 10){
    timerCounter = 0;
    updateTime();
  }
  delay(1000);
}

void updateTime(){
  unsigned int tempBTime = 0;
  long dTime = getSecondsInDay();
  long h = 43200;
  for(int i = 0; i < 8; i++){
    if(dTime / h > 0){
      tempBTime++;
      dTime -= h;
    }
    if(i != 7) tempBTime = tempBTime << 1;
    h /= 2;
  }
  if(tempBTime != binaryTime){
    binaryTime = tempBTime;
    shiftOut(dataPin, clockPin, MSBFIRST, binaryTime);
  }
}

void enterEditMode(){
  flashLEDs();
  int hourUpperDigit = getNumberInput(2);
  flashLEDs();
  int hourLowerDigit = 0;
  if(hourUpperDigit >= 2) hourLowerDigit = getNumberInput(3);
  else hourLowerDigit = getNumberInput(9);
  flashLEDs();
  int minutesUpperDigit = getNumberInput(5);
  flashLEDs();
  int minutesLowerDigit = getNumberInput(9);
  flashLEDs();
  int hours = (hourUpperDigit << 4) + hourLowerDigit;
  int minutes = (minutesUpperDigit << 4) + minutesLowerDigit;
  TinyWireM.beginTransmission(DS1307_ADDR);
  TinyWireM.send(0);
  // 0 Seconds
  TinyWireM.send(0b00000000);
  TinyWireM.send(minutes);
  TinyWireM.send(hours);
  //Set the rest to the 25/4/2013
  TinyWireM.send(0b00000100);
  TinyWireM.send(0b00100101);
  TinyWireM.send(0b00000100);
  TinyWireM.send(0b0010000000010011);
  TinyWireM.send(0);
  TinyWireM.endTransmission();
}

int getNumberInput(int upperLimit){
  int counter = 0;
  int numberSoFar = 0;
  while(counter < 20){
    if(buttonPressed()){
      counter++;
    }
    else if(counter > 0 && counter < 20){
      counter = 0;
      if(++numberSoFar > upperLimit) numberSoFar = 0;
      shiftOut(dataPin, clockPin, MSBFIRST, numberSoFar);
    }  
    delay(100);
  }
  return numberSoFar;
}

void flashLEDs(){
  shiftOut(dataPin, clockPin, MSBFIRST, 0b10101010);
  delay(333);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b01010101);
  delay(333);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b10101010);
  delay(333);
  shiftOut(dataPin, clockPin, MSBFIRST, 0);
}

boolean buttonPressed(){
    if(digitalRead(buttonPin) == HIGH) return true;
    return false;
}


long getSecondsInDay(){
  long totalSeconds = 0;
  TinyWireM.beginTransmission(DS1307_ADDR); 
  TinyWireM.send(0); 
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(DS1307_ADDR, 3);
  long seconds = bcdToDec(TinyWireM.receive());
  long minutes = bcdToDec(TinyWireM.receive());
  long hours = bcdToDec(TinyWireM.receive());
  totalSeconds = seconds + (minutes * 60) + (hours * 60 * 60);
  return totalSeconds;
}

// Interrupt method for the button
ISR(PCINT0_vect){
  cli();
  byte counter = 0;
  while(buttonPressed()){
    if(counter++ >= 20){
      enterEditMode();
      binaryTime = 0;
      updateTime();
      break;
    }
    delay(100);
  }
  sei();
}

byte bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}
