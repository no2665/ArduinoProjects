#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

Adafruit_ST7735 tft = Adafruit_ST7735(10, 9, 8);

const int squaresX = 4;
const int squaresY = 80;
word squares[squaresX][squaresY];

word topLineBuffer[squaresX];
word buffer1[squaresX];
word buffer2[squaresX];

int redButton = 6;
int yellowButton = 5;

boolean running = true;
boolean redDown = false;
boolean hasPrev = false;

int prevX = 0;
int prevY = 0;

void setup(){
  randomSeed(analogRead(0));
  pinMode(redButton, INPUT);
  pinMode(yellowButton, INPUT);
  
  *digitalPinToPCMSK(redButton) |= bit (digitalPinToPCMSKbit(redButton));
  PCIFR |= bit (digitalPinToPCICRbit(redButton));
  PCICR |= bit (digitalPinToPCICRbit(redButton));
  
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);  
  
  for(int x = 0; x < squaresX; x++){
    topLineBuffer[x] = 0;
    buffer1[x] = 0;
    buffer2[x] = 0;
    for(int y = 0; y < squaresY; y++){
      for(int b = 0; b < 16; b++){
        int r = random(10);
        if(r > 7){
          // Set to 1
          squares[x][y] = squares[x][y] | (1 << b);
        }
      }
    }
  }
  
  drawGame();
}

void loop(){
  
  if(redButtonPressed()){
    running = !running;
    hasPrev = false;
  }
  
  if(running){
    playGame();
    drawGame();
  } else {
    int x = analogRead(0);
    int y = analogRead(1);
    int xAxis = map(x, 240, 880, 0, (int) tft.width() - 1);
    int yAxis = map(y, 175, 765, 0, (int) tft.height() - 1);
    xAxis = xAxis - (xAxis % 2);
    yAxis = yAxis - (yAxis % 2);
    if(digitalRead(yellowButton)){
      int sx = calculateX(xAxis);
      int sy = calculateY(yAxis);
      int sb = calculateB(xAxis);
      squares[sx][sy] = squares[sx][sy] | (1 << sb);
    }
    //Serial.print(x);
    //Serial.print(", ");
    //Serial.println(y);
    tft.fillRect(xAxis, yAxis, 2, 2, ST7735_RED);
    if(hasPrev){
      if(!(xAxis == prevX && yAxis == prevY)){
        int square = (squares[calculateX(prevX)][calculateY(prevY)] >> calculateB(prevX)) & 1;
        if(square > 0){
          tft.fillRect(prevX, prevY, 2, 2, ST7735_YELLOW);
        } else {
          tft.fillRect(prevX, prevY, 2, 2, ST7735_BLACK);
        }
      }
    }
    prevX = xAxis;
    prevY = yAxis;
    hasPrev = true;
  }
}

void playGame(){
  // Do the top line first
  for(int x = 0; x < squaresX; x++){
    topLineBuffer[x] = 0;
    for(int b = 15; b >= 0; b--){
      byte n = calculateNeighbours(x, 0, b);
      int square = (squares[x][0] >> b) & 1;
      if(square > 0){
        if(!(n < 2 || n > 3)) topLineBuffer[x] = topLineBuffer[x] | (1 << b);
      } else {
        if(n == 3) topLineBuffer[x] = topLineBuffer[x] | (1 << b);
      }
    }
  }
    
  word* currentBuffer;  
  for(int y = 1; y < squaresY; y++){
    if(y % 2 == 1){
      currentBuffer = buffer1;
    } else {
      currentBuffer = buffer2;
    }
    for(int x = 0; x < squaresX; x++){
      currentBuffer[x] = 0;
      for(int b = 15; b >= 0; b--){
        byte n = calculateNeighbours(x, y, b);
        int square = (squares[x][y] >> b) & 1;
        if(square > 0){
          if(!(n < 2 || n > 3)) currentBuffer[x] = currentBuffer[x] | ((word) 1 << b);
        } else {
          if(n == 3) currentBuffer[x] = currentBuffer[x] | ((word) 1 << b);
        }
      }
    }
    if(y % 2 == 0){
      for(int i = 0; i < squaresX; i++){
        squares[i][y-1] = buffer1[i];
      }
    } else if(y != 1 && y % 2 == 1){
      for(int i = 0; i < squaresX; i++){
        squares[i][y-1] = buffer2[i];
      }
    }
  }

  for(int i = 0; i < squaresX; i++){
    squares[i][squaresY-1] = currentBuffer[i];
  }
  
  for(int i = 0; i < squaresX; i++){
    squares[i][0] = topLineBuffer[i];
  }
  
}

byte calculateNeighbours(word x, int y, int b){
  byte n = 0;
  for(int B = b + 1; B > b - 2; B--){
    int adjB = B;
    int adjX = x;
    if(B > 15){
      adjB = 0;
      if(x <= 0){
        adjX = squaresX - 1;
      } else {
        adjX = adjX - 1;
      }
    } else if(B < 0){
      adjB = 15;
      if(x >= squaresX - 1){
        adjX = 0;
      } else {
        adjX = adjX + 1;
      }
    }
    for(int Y = y - 1; Y < y + 2; Y++){
      int adjY = Y;
      if(Y < 0) adjY = squaresY - 1;
      if(Y >= squaresY) adjY = 0;
      
      if(!(B == b && Y == y && adjX == x)){
        int square = (squares[adjX][adjY] >> adjB) & 1;
        if(square > 0) n++;
      }
    }
  }
  return n;
}

void drawGame(){
  for(int y = 0; y < squaresY; y++){
    for(int x = 0; x < squaresX; x++){
      //Something wrong here?
      for(int b = 15; b >= 0; b--){
        int square = (squares[x][y] >> b) & 1;
        int newB = map(b, 15, 0, 0, 15);
        if(square > 0){
          tft.fillRect((x * 32) + (newB * 2), y*2, 2, 2, ST7735_YELLOW);
        }
        else {
          tft.fillRect((x * 32) + (newB * 2), y*2, 2, 2, ST7735_BLACK);
        }
      }
    }
  }
}

byte calculateX(int x){
  return x/32;
}

byte calculateB(int x){
  byte val = (x - (calculateX(x) * 32))/2;
  return map(val, 0, 15, 15, 0);
}

byte calculateY(int y){
  return y/2;
}

boolean redButtonPressed(){
  if(digitalRead(redButton)){
    redDown = true;
  } else if(redDown){
    redDown = false;
    return true;
  }
  return false;
}

boolean yellowButtonPressed(){
  static boolean down = false;
  if(digitalRead(yellowButton)){
    down = true;
  } else if(down){
    down = false;
    return true;
  }
  return false;
}

ISR (PCINT2_vect){
  if(running && digitalRead(redButton)) redDown = true;
}  


unsigned int bitOut(void)
{
  static unsigned long firstTime=1, prev=0;
  unsigned long bit1=0, bit0=0, x=0, port=0, limit=99;
  if (firstTime)
  {
    firstTime=0;
    prev=analogRead(port);
  }
  while (limit--)
  {
    x=analogRead(port);
    bit1=(prev!=x?1:0);
    prev=x;
    x=analogRead(port);
    bit0=(prev!=x?1:0);
    prev=x;
    if (bit1!=bit0)
      break;
  }
  return bit1;
}
//------------------------------------------------------------------------------
unsigned long seedOut(unsigned int noOfBits)
{
  // return value with 'noOfBits' random bits set
  unsigned long seed=0;
  for (int i=0;i<noOfBits;++i)
    seed = (seed<<1) | bitOut();
  return seed;
}
