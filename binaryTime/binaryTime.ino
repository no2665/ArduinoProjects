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
byte timerCounter = 1;

void setup () {
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  TinyWireM.begin();
  GIMSK=(1 << PCIE); //Enable Pin Change Int 
  PCMSK=(1 << PCINT1); //Set PCINT1 to cause interrupt
  setup_watchdog(9);
}

void loop () {
  if(timerCounter >= 15){
    timerCounter = 0;
    updateTime();
  }
  if(buttonInterrupt){
    buttonInterrupt = false;
    byte counter = 0;
    while(buttonPressed()){
      if(++counter >= 20){
        enterEditMode();
        binaryTime = 0;
        updateTime();
        break;
      }
      delay(100);
    }
  }
    /*for(int i = 0; i < 50; i++){
      if(buttonPressed()){
        if(++bCounter >= 20){
          enterEditMode();
          bCounter = 0;
          binaryTime = 0;
          break;
        }  
      }
      else{
        bCounter = 0;
      }  
      delay(100);
    }*/
  sei();
  system_sleep();
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
    else return false;
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
  buttonInterrupt = true;
}

ISR(WDT_vect) {
  timerCounter++;
}

void system_sleep() {
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
}

byte bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
