#include <Wire.h>
#include <AFMotor.h>

#define srfAddress 0x70
#define cmbByte 0x00

int distance = 0;
byte highByte = 0x00;
byte lowByte = 0x00;

const int ldrPin = 0;
const int ledPin = 1;
const int buttonPin = 9;
int lastButtonState = HIGH;
boolean pressed = false;

AF_DCMotor leftMotor(3);
AF_DCMotor rightMotor(4);

int middleSpeed = 128;
int fastestSpeed = 255;

void setup(){
  Wire.begin();
  delay(100);
  
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  leftMotor.setSpeed(middleSpeed);
  rightMotor.setSpeed(middleSpeed);
}

void loop(){
  updateLights();
  boolean p = checkButton();
  if(p){
    int distance = getRange();
    while(distance > 30 && p){
      leftMotor.run(FORWARD);
      rightMotor.run(FORWARD); 
      updateLights();
      distance = getRange();
      p = checkButton();
    }
    if(p){
      brake(true);
      delay(1000);
      turnLeft();
    }
    else{
      delay(500);
    }
  }
}

void turnLeft(){
  leftMotor.run(RELEASE);
  rightMotor.run(RELEASE);
  leftMotor.setSpeed(middleSpeed / 2);
  rightMotor.setSpeed(middleSpeed / 2);
  rightMotor.run(BACKWARD);
  leftMotor.run(FORWARD);
  delay(1000);
  leftMotor.run(RELEASE);
  rightMotor.run(RELEASE);
  leftMotor.setSpeed(middleSpeed);
  rightMotor.setSpeed(middleSpeed);
}

void brake(boolean forward){
   leftMotor.run(RELEASE);
   rightMotor.run(RELEASE);
   leftMotor.setSpeed(middleSpeed / 2);
   rightMotor.setSpeed(middleSpeed / 2);
   if(forward){
     leftMotor.run(BACKWARD);
     rightMotor.run(BACKWARD);
   }
   else{
     leftMotor.run(FORWARD);
     rightMotor.run(FORWARD);
   }
   delay(250);
   leftMotor.run(RELEASE);
   rightMotor.run(RELEASE);
   leftMotor.setSpeed(middleSpeed);
   rightMotor.setSpeed(middleSpeed);
}

int getRange(){
  int reading = 0;
  Wire.beginTransmission(112); // transmit to device #112 (0x70)
  // the address specified in the datasheet is 224 (0xE0)
  // but i2c adressing uses the high 7 bits so it's 112
  Wire.write(byte(0x00));      // sets register pointer to the command register (0x00)  
  Wire.write(byte(0x51));      // command sensor to measure in "inches" (0x50) 
                               // use 0x51 for centimeters
                               // use 0x52 for ping microseconds
  Wire.endTransmission();      // stop transmitting

  // step 2: wait for readings to happen
  delay(70);                   // datasheet suggests at least 65 milliseconds

  // step 3: instruct sensor to return a particular echo reading
  Wire.beginTransmission(112); // transmit to device #112
  Wire.write(byte(0x02));      // sets register pointer to echo #1 register (0x02)
  Wire.endTransmission();      // stop transmitting

  // step 4: request reading from sensor
  Wire.requestFrom(112, 2);    // request 2 bytes from slave device #112

  // step 5: receive reading from sensor
  if(2 <= Wire.available())    // if two bytes were received
  {
    reading = Wire.read();  // receive high byte (overwrites previous reading)
    reading = reading << 8;    // shift high byte to be high 8 bits
    reading |= Wire.read(); // receive low byte as lower 8 bits
  }
  return reading;
}

void updateLights(){
  int reading = analogRead(ldrPin);
  if(reading < 13) digitalWrite(ledPin, HIGH);
  else digitalWrite(ledPin, LOW);
}

boolean checkButton(){
  int reading = digitalRead(buttonPin);
  if(reading == LOW && lastButtonState == HIGH){
    pressed = !pressed;
    lastButtonState = reading;
  }
  else if(reading == HIGH  && lastButtonState == LOW){
    lastButtonState = reading; 
  }
  return pressed;
}
