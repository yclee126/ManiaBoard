// Created by yclee126 for maniaboard project V5.4
/*

  ===== CONTROLS =====
           +----+----+
  (Arduino)| K1 | K2 | K7
 +----+----+----+----+
 | K3 | K4 | K5 | K6 | K8
 +----+----+----+----+

 K7 short press : K7
 K7 long press : Reset kps display(K2 led)
 K8 short press : K8
 K8 long press : Cycle keymaps
 K7 & K8 short press : Enter settings (open notepad to print)
 K7 & K8 long press : Cycle led mode

*/

#include "Maniaboard.h"
#include "usb_hid_keys.h"
#include "EEPROM.h"

//#define DEBUGMODE

#define LED_PIN     10
#define NUM_LEDS    6

byte leds[NUM_LEDS][3] = {0,};// G R B
char indexChar = 'e'; //cycle this char between uppercase and lowercase if the code change has to be applied to the EEPROM

byte const keyCount = 8;
byte const slotLimit = 10; //Maximum slots (affects initGradient, key, mod)
byte slotCount = 5; //number of slots
byte slot = 0;
byte realKeyValue[2];
byte realModValue[2];
boolean enterSettings;

int dataAdr;
int indexAdr;

const byte strBufferSize = 200;
char strBuffer[strBufferSize]; // <=256
#define msg(stw) strlen(stw) < strBufferSize ? strcpy_P(strBuffer, PSTR(stw)) : strcpy_P(strBuffer, PSTR("**ERROR : The msg is too long! SKIPPED.")), typemsg()
//      |------| |---------------------------?--------------------------------:--------------------------------------------------------------------|  |-------|
//Still I can't understand why "PSTR("**ERROR" part only takes the program storage as a single line, since it's in #define it should take multiple times as msg() is used!!
//comma used in there makes two expression bond together, so msg("something") returns typemsg()

#define str(stw) strlen(stw) < strBufferSize ? strcpy_P(strBuffer, PSTR(stw)) : strcpy_P(strBuffer, PSTR("**ERROR : The str is too long! SKIPPED.")), typestr()

byte typeSpeedChar = 12; //12 : allow multiple keystrokes, 1: type one by one
boolean turboMode = false; // support repeating multiple key presses?
//cmd 한글출력시에는 typeSpeed = 0 같이 설정해야함!!
int spaceMod = 0;
byte typeSpeed = typeSpeedChar - 1;
boolean constTurboMode = turboMode;

byte keyShade[6] = {0,};
byte const gradientCount = 6;
byte ledMode = 0;
/*
 * 0 = Bright mode
 * 1 = Night mode (all off except for the kps indicator and pressed keys)
 * 2 = All off
 */

struct{
  byte r;
  byte g;
  byte b;
}gradient[gradientCount];

struct{
  byte r;
  byte g;
  byte b;
}initGradient[slotLimit];

// !bitRead(PIND, 3)// TX
// !bitRead(PIND, 2)// RX
// !bitRead(PIND, 1)// 02
// !bitRead(PIND, 0)// 03
// !bitRead(PIND, 4)// 04
// !bitRead(PINC, 6)// 05
// !bitRead(PIND, 7)// 06
// !bitRead(PINE, 6)// 07
// !bitRead(PINB, 4)// 08
// !bitRead(PINB, 5)// 09
// !bitRead(PINB, 6)// 10
// !bitRead(PINB, 2)// 16
// !bitRead(PINB, 3)// 14
// !bitRead(PINB, 1)// 15
// !bitRead(PINF, 7)// A0
// !bitRead(PINF, 6)// A1
// !bitRead(PINF, 5)// A2
// !bitRead(PINF, 4)// A3
/*
PORTX ->true = pullup or high, false = tristate or low
DDRX  ->true = output, false = input
PINX  ->pin reading value return
*/

#define KEY0INPUT !bitRead(PIND, 1)// 02
#define KEY1INPUT !bitRead(PIND, 0)// 03
#define KEY2INPUT !bitRead(PIND, 4)// 04
#define KEY3INPUT !bitRead(PINC, 6)// 05
#define KEY4INPUT !bitRead(PIND, 7)// 06
#define KEY5INPUT !bitRead(PINE, 6)// 07
#define FUNC0INPUT !bitRead(PINB, 3)// 14
#define FUNC1INPUT !bitRead(PINB, 1)// 15

//key pins
byte const pin[keyCount] = {2, 3, 4, 5, 6, 7, 14, 15};

byte key[slotLimit][keyCount] = {0,};
byte mod[slotLimit][keyCount] = {0,};
boolean keyState[3][keyCount] = {0,};

unsigned long prevTime = millis();
boolean ledPower = false;
char latch = 's'; //to prevent "new maxKps" effect triggering at startup
/*
 * 'm' = maxkps clear
 * 's' = slot change
 * 'l' = led mode change
 */
byte kps = 0;
byte kpsAdd = 0;
int timeQueue[100] = {0,};
byte numQueue[100] = {0,};
int writePointer = 0;
int readPointer = 0;
byte queueCount = 0;
struct{
  boolean init = true;
  byte kps;
  byte r;
  byte g;
  byte b;
}maxKps;

int queueTime = millis() & long(2047);

byte const additionalSpace = (keyCount+3) * slotLimit;

void setup() {

  pinMode(LED_PIN, OUTPUT);
  showLed();

  {//default setting
  //slot 1 //Osu! mode
  initGradient[0] = {40, 17, 12};
  key[0][0] = KEY_UP;
  mod[0][0] = 0;
  key[0][1] = KEY_ESC;
  mod[0][1] = 0;
  key[0][2] = KEY_F2;
  mod[0][2] = 0;
  key[0][3] = KEY_LEFT;
  mod[0][3] = 0;
  key[0][4] = KEY_DOWN;
  mod[0][4] = 0;
  key[0][5] = KEY_RIGHT;
  mod[0][5] = 0;
  key[0][6] = KEY_SPACE;
  mod[0][6] = 0;
  key[0][7] = KEY_F10;
  mod[0][7] = 0;
                               
  //slot 2 //Bonk.io, tagpro.gg mode + copypaste
  initGradient[1] = {40, 60, 60};
  key[1][0] = KEY_UP;
  mod[1][0] = 0;
  key[1][1] = KEY_ESC;
  mod[1][1] = 0;
  key[1][2] = KEY_X;
  mod[1][2] = 0;
  key[1][3] = KEY_LEFT;
  mod[1][3] = 0;
  key[1][4] = KEY_DOWN;
  mod[1][4] = 0;
  key[1][5] = KEY_RIGHT;
  mod[1][5] = 0;
  key[1][6] = KEY_V;
  mod[1][6] = KEY_MOD_LCTRL;
  key[1][7] = KEY_C;
  mod[1][7] = KEY_MOD_LCTRL;
                               
  //slot 3 //WASD mode
  initGradient[2] = {40, 80, 20};
  key[2][0] = KEY_W;
  mod[2][0] = 0;
  key[2][1] = KEY_ENTER;
  mod[2][1] = 0;
  key[2][2] = KEY_SPACE;
  mod[2][2] = 0;
  key[2][3] = KEY_A;
  mod[2][3] = 0;
  key[2][4] = KEY_S;
  mod[2][4] = 0;
  key[2][5] = KEY_D;
  mod[2][5] = 0;
  key[2][6] = KEY_EQUAL; //with mod enabled
  mod[2][6] = KEY_MOD_LSHIFT;
  key[2][7] = KEY_MINUS;
  mod[2][7] = 0;
                               
  //slot 4 //undertale mode
  initGradient[3] = {40, 1, 1};
  key[3][0] = KEY_UP;
  mod[3][0] = 0;
  key[3][1] = KEY_X;
  mod[3][1] = 0;
  key[3][2] = KEY_Z;
  mod[3][2] = 0;
  key[3][3] = KEY_LEFT;
  mod[3][3] = 0;
  key[3][4] = KEY_DOWN;
  mod[3][4] = 0;
  key[3][5] = KEY_RIGHT;
  mod[3][5] = 0;
  key[3][6] = KEY_ESC;
  mod[3][6] = 0;
  key[3][7] = KEY_C;
  mod[3][7] = 0;
                               
  //slot 5 //test mode
  initGradient[4] = {100, 20, 10};
  key[4][0] = 0;
  mod[4][0] = 0;
  key[4][1] = 0;
  mod[4][1] = 0;
  key[4][2] = 0;
  mod[4][2] = 0;
  key[4][3] = 0;
  mod[4][3] = 0;
  key[4][4] = 0;
  mod[4][4] = 0;
  key[4][5] = 0;
  mod[4][5] = 0;
  key[4][6] = 0;
  mod[4][6] = 0;
  key[4][7] = 0;
  mod[4][7] = 0;
  }
  
  byte full = 100;
  
  gradient[0].r = initGradient[0].r;
  gradient[0].g = initGradient[0].g;
  gradient[0].b = initGradient[0].b;
  gradient[1] = {0, full, 0};
  gradient[2] = {full, full , 0};
  gradient[3] = {255, 103, 154};
  gradient[4] = {full, 0, full};
  gradient[5] = {255, 0, 0};
  
  if(checkEEPROM(indexChar)){ //the data is valid
    leds[1][1] = 200; //Green
    showLed();
    delay(500);
    ledMode = EEPROM.read(indexAdr+1);
    for(byte i = 0; i < slotLimit; i++){
      for(byte k = 0; k < keyCount; k++){
        key[i][k] = EEPROM.read( dataAdr + k + keyCount*0 + i*(keyCount*2 + 3) );
        mod[i][k] = EEPROM.read( dataAdr + k + keyCount*1 + i*(keyCount*2 + 3) );
      }
      initGradient[i].r = EEPROM.read( dataAdr + keyCount*2 + 0 + i*(keyCount*2 + 3) );
      initGradient[i].g = EEPROM.read( dataAdr + keyCount*2 + 1 + i*(keyCount*2 + 3) );
      initGradient[i].b = EEPROM.read( dataAdr + keyCount*2 + 2 + i*(keyCount*2 + 3) );
    }
  }
  else{ //invalid data, writing new data
    leds[1][0] = 200; //Red
    showLed();
    delay(500);
    EEPROM.update(indexAdr+1, ledMode);
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.update(i, 0);
      analogRead(A1);
    }
    randomSeed(analogRead(A1));
    indexAdr = random(0,EEPROM.length()-(keyCount*slotLimit*2) );
    dataAdr = indexAdr + 2;
    EEPROM.update(indexAdr, indexChar);
    for(byte i = 0; i < slotLimit; i++){
      writeKeyMod(i);
      writeInitGradient(i);
    }
  }
  
  //used when the function key disabled as a normal key
  backupFuncKeys();
  
  for(byte i = 0; i < keyCount; i++){
    pinMode(pin[i], INPUT_PULLUP);
  }
  
  pinMode(A3, OUTPUT);

  maxKps.kps = 0;
  maxKps.r = initGradient[0].r;
  maxKps.g = initGradient[0].g;
  maxKps.b = initGradient[0].b;
  
  Keyboard.begin();
  #ifdef DEBUGMODE
  Serial.begin(115200);
  #endif
  
}

// Polling rate must be measured as double of A3's output!!

void loop() {
  
  keyState[0][0] = KEY0INPUT;
  if(keyState[0][0] == keyState[1][0]) if(keyState[2][0] == !keyState[0][0]) keyState[0][0] = !keyState[0][0];
  keyState[0][1] = KEY1INPUT;
  if(keyState[0][1] == keyState[1][1]) if(keyState[2][1] == !keyState[0][1]) keyState[0][1] = !keyState[0][1];
  keyState[0][2] = KEY2INPUT;
  if(keyState[0][2] == keyState[1][2]) if(keyState[2][2] == !keyState[0][2]) keyState[0][2] = !keyState[0][2];
  keyState[0][3] = KEY3INPUT;
  if(keyState[0][3] == keyState[1][3]) if(keyState[2][3] == !keyState[0][3]) keyState[0][3] = !keyState[0][3];
  keyState[0][4] = KEY4INPUT;
  if(keyState[0][4] == keyState[1][4]) if(keyState[2][4] == !keyState[0][4]) keyState[0][4] = !keyState[0][4];
  keyState[0][5] = KEY5INPUT;
  if(keyState[0][5] == keyState[1][5]) if(keyState[2][5] == !keyState[0][5]) keyState[0][5] = !keyState[0][5];
  keyState[0][6] = FUNC0INPUT;
  if(keyState[0][6] == keyState[1][6]) if(keyState[2][6] == !keyState[0][6]) keyState[0][6] = !keyState[0][6];
  keyState[0][7] = FUNC1INPUT;
  if(keyState[0][7] == keyState[1][7]) if(keyState[2][7] == !keyState[0][7]) keyState[0][7] = !keyState[0][7];
  Keyboard.packet(
    mod[slot][0]*keyState[0][0] | mod[slot][1]*keyState[0][1] | mod[slot][2]*keyState[0][2] | mod[slot][3]*keyState[0][3] | mod[slot][4]*keyState[0][4] | mod[slot][5]*keyState[0][5] | mod[slot][6]*keyState[0][6] | mod[slot][7]*keyState[0][7],
    key[slot][0]*keyState[0][0],
    key[slot][1]*keyState[0][1],
    key[slot][2]*keyState[0][2],
    key[slot][3]*keyState[0][3],
    key[slot][4]*keyState[0][4],
    key[slot][5]*keyState[0][5],
    key[slot][6]*keyState[0][6],
    key[slot][7]*keyState[0][7],
    0,
    0,
    0,
    0
  );
  
  checkFunc(0);                                                                                                                                                               

  PORTF ^= B00010000;
  
  keyState[1][0] = KEY0INPUT;
  if(keyState[1][0] == keyState[2][0]) if(keyState[0][0] == !keyState[1][0]) keyState[1][0] = !keyState[1][0];
  keyState[1][1] = KEY1INPUT;
  if(keyState[1][1] == keyState[2][1]) if(keyState[0][1] == !keyState[1][1]) keyState[1][1] = !keyState[1][1];
  keyState[1][2] = KEY2INPUT;
  if(keyState[1][2] == keyState[2][2]) if(keyState[0][2] == !keyState[1][2]) keyState[1][2] = !keyState[1][2];
  keyState[1][3] = KEY3INPUT;
  if(keyState[1][3] == keyState[2][3]) if(keyState[0][3] == !keyState[1][3]) keyState[1][3] = !keyState[1][3];
  keyState[1][4] = KEY4INPUT;
  if(keyState[1][4] == keyState[2][4]) if(keyState[0][4] == !keyState[1][4]) keyState[1][4] = !keyState[1][4];
  keyState[1][5] = KEY5INPUT;
  if(keyState[1][5] == keyState[2][5]) if(keyState[0][5] == !keyState[1][5]) keyState[1][5] = !keyState[1][5];
  keyState[1][6] = FUNC0INPUT;
  if(keyState[1][6] == keyState[2][6]) if(keyState[0][6] == !keyState[1][6]) keyState[1][6] = !keyState[1][6];
  keyState[1][7] = FUNC1INPUT;
  if(keyState[1][7] == keyState[2][7]) if(keyState[0][7] == !keyState[1][7]) keyState[1][7] = !keyState[1][7];
  Keyboard.packet(
    mod[slot][0]*keyState[1][0] | mod[slot][1]*keyState[1][1] | mod[slot][2]*keyState[1][2] | mod[slot][3]*keyState[1][3] | mod[slot][4]*keyState[1][4] | mod[slot][5]*keyState[1][5] | mod[slot][6]*keyState[1][6] | mod[slot][7]*keyState[1][7],
    key[slot][0]*keyState[1][0],
    key[slot][1]*keyState[1][1],
    key[slot][2]*keyState[1][2],
    key[slot][3]*keyState[1][3],
    key[slot][4]*keyState[1][4],
    key[slot][5]*keyState[1][5],
    key[slot][6]*keyState[1][6],
    key[slot][7]*keyState[1][7],
    0,
    0,
    0,
    0
  );
  
  checkFunc(1);

  PORTF ^= B00010000;
  
  keyState[2][0] = KEY0INPUT;
  if(keyState[2][0] == keyState[0][0]) if(keyState[1][0] == !keyState[2][0]) keyState[2][0] = !keyState[2][0];
  keyState[2][1] = KEY1INPUT;
  if(keyState[2][1] == keyState[0][1]) if(keyState[1][1] == !keyState[2][1]) keyState[2][1] = !keyState[2][1];
  keyState[2][2] = KEY2INPUT;
  if(keyState[2][2] == keyState[0][2]) if(keyState[1][2] == !keyState[2][2]) keyState[2][2] = !keyState[2][2];
  keyState[2][3] = KEY3INPUT;
  if(keyState[2][3] == keyState[0][3]) if(keyState[1][3] == !keyState[2][3]) keyState[2][3] = !keyState[2][3];
  keyState[2][4] = KEY4INPUT;
  if(keyState[2][4] == keyState[0][4]) if(keyState[1][4] == !keyState[2][4]) keyState[2][4] = !keyState[2][4];
  keyState[2][5] = KEY5INPUT;
  if(keyState[2][5] == keyState[0][5]) if(keyState[1][5] == !keyState[2][5]) keyState[2][5] = !keyState[2][5];
  keyState[2][6] = FUNC0INPUT;
  if(keyState[2][6] == keyState[0][6]) if(keyState[1][6] == !keyState[2][6]) keyState[2][6] = !keyState[2][6];
  keyState[2][7] = FUNC1INPUT;
  if(keyState[2][7] == keyState[0][7]) if(keyState[1][7] == !keyState[2][7]) keyState[2][7] = !keyState[2][7];
  Keyboard.packet(
    mod[slot][0]*keyState[2][0] | mod[slot][1]*keyState[2][1] | mod[slot][2]*keyState[2][2] | mod[slot][3]*keyState[2][3] | mod[slot][4]*keyState[2][4] | mod[slot][5]*keyState[2][5] | mod[slot][6]*keyState[2][6] | mod[slot][7]*keyState[2][7],
    key[slot][0]*keyState[2][0],
    key[slot][1]*keyState[2][1],
    key[slot][2]*keyState[2][2],
    key[slot][3]*keyState[2][3],
    key[slot][4]*keyState[2][4],
    key[slot][5]*keyState[2][5],
    key[slot][6]*keyState[2][6],
    key[slot][7]*keyState[2][7],
    0,
    0,
    0,
    0
  );
  
  checkFunc(2);

  PORTF ^= B00010000;
  
}

void writeKeyMod(byte i){
  for(byte k = 0; k < keyCount; k++){
    EEPROM.update(dataAdr + k + keyCount*0 + i*(keyCount*2 + 3), key[i][k]);
    EEPROM.update(dataAdr + k + keyCount*1 + i*(keyCount*2 + 3), mod[i][k]);
  }
}

void writeInitGradient(byte i){
  EEPROM.update(dataAdr + keyCount*2 + 0 + i*(keyCount*2 + 3), initGradient[i].r);
  EEPROM.update(dataAdr + keyCount*2 + 1 + i*(keyCount*2 + 3), initGradient[i].g);
  EEPROM.update(dataAdr + keyCount*2 + 2 + i*(keyCount*2 + 3), initGradient[i].b);
}

//byte skps;

void checkFunc(byte num){
  
  kpsAdd = 0;
  byte prevNum = (num+2)%3;
  for(byte i = 0; i < 6; i++){ //does not include function keys
    if(keyState[prevNum][i] == false && keyState[num][i] == true) kpsAdd++;
  }

  queueTime = millis() & long(2047);

  writePointer %= 100;
  readPointer %= 100;
  
  if(kpsAdd != 0){ //add kps info to the queue
    timeQueue[writePointer] = queueTime;
    numQueue[writePointer] = kpsAdd;
    
    kps += kpsAdd;
    writePointer++;
    queueCount++;
  }
  
  //while(1){ //remove kps info from the queue
    int diff = queueTime - timeQueue[readPointer];
    
    if( ((diff > 0 && diff > 999) || (diff < 0 && diff > -1049)) && queueCount != 0 ){
      kps -= numQueue[readPointer];
      readPointer++;
      queueCount--;
    }
  //  else break;
  //} //somehow while() causes bug. but I think it's correct, so I left that code into remark.

  byte r = 255;
  byte g = 0;
  byte b = 0; 

  byte shadeLevel = 4;
  byte gCount = 6; //number of gradients
  byte gNum = kps/shadeLevel;
  byte cKps = kps%shadeLevel;
  if(gNum < gCount-1){
    r = map(cKps, 0, shadeLevel, gradient[gNum].r, gradient[gNum+1].r);
    g = map(cKps, 0, shadeLevel, gradient[gNum].g, gradient[gNum+1].g);
    b = map(cKps, 0, shadeLevel, gradient[gNum].b, gradient[gNum+1].b);
  }
  else{
    r = gradient[gCount-1].r;
    g = gradient[gCount-1].g;
    b = gradient[gCount-1].b;
  }
  
  byte keyShadeLevel = 80; //the shade of color when the key is pressed. (big number means longer glowing time)
  struct{
    byte r = 200;
    byte g = 200;
    byte b = 200;
  }pressedcolor;
  
  if(maxKps.kps < kps || maxKps.init == true){
    if(latch != 's'){
      keyShade[1] = keyShadeLevel;
    }
    maxKps.init = false;
    maxKps.kps = kps;
    maxKps.r = r;
    maxKps.g = g;
    maxKps.b = b;
  }

  if(latch == 's'){ //////////////////////////////////////NEED OPTIMIZATION
    byte gNum = maxKps.kps/shadeLevel;
    byte cKps = maxKps.kps%shadeLevel;
    if(gNum < gCount-1){
      maxKps.r = map(cKps, 0, shadeLevel, gradient[gNum].r, gradient[gNum+1].r);
      maxKps.g = map(cKps, 0, shadeLevel, gradient[gNum].g, gradient[gNum+1].g);
      maxKps.b = map(cKps, 0, shadeLevel, gradient[gNum].b, gradient[gNum+1].b);
    }
    else{
      maxKps.r = gradient[gCount-1].r;
      maxKps.g = gradient[gCount-1].g;
      maxKps.b = gradient[gCount-1].b;
    }
    
    if(ledMode == 1 || ledMode == 2){ //show shadecolor
      for(byte i = 0; i < 6; i++){
        keyShade[i] = keyShadeLevel;
      }
    }
    else if(ledMode == 3){
      keyShade[0] = keyShadeLevel;
    }
  }

  switch(ledMode){
    
    case 0:
      for(byte i = 0; i < 6; i++){
        if(keyShade[i] != 0) keyShade[i] --; //restore to original color over time
        if(keyState[num][i]) keyShade[i] = keyShadeLevel; //if pressed
        
        if(i == 1){ //kps indicator
          leds[i][0] = map(keyShade[i], 0, keyShadeLevel, maxKps.r, 255-r);
          leds[i][1] = map(keyShade[i], 0, keyShadeLevel, maxKps.g, 255-g);
          leds[i][2] = map(keyShade[i], 0, keyShadeLevel, maxKps.b, 255-b);
        }
        else{ //other keys
          leds[i][0] = map(keyShade[i], 0, keyShadeLevel, r, 255-r);
          leds[i][1] = map(keyShade[i], 0, keyShadeLevel, g, 255-g);
          leds[i][2] = map(keyShade[i], 0, keyShadeLevel, b, 255-b);
        }
      }
      break;
      
    case 1:
      for(byte i = 0; i < 6; i++){
        if(keyShade[i] != 0) keyShade[i] --; //restore to original color over time
        if(keyState[num][i]) keyShade[i] = keyShadeLevel; //if pressed
        
        if(i == 1){ //kps indicator
          leds[i][0] = map(keyShade[i], 0, keyShadeLevel, maxKps.r, 255-r);
          leds[i][1] = map(keyShade[i], 0, keyShadeLevel, maxKps.g, 255-g);
          leds[i][2] = map(keyShade[i], 0, keyShadeLevel, maxKps.b, 255-b);
        }
        else{ //other keys
          leds[i][0] = map(keyShade[i], 0, keyShadeLevel, 0, r);
          leds[i][1] = map(keyShade[i], 0, keyShadeLevel, 0, g);
          leds[i][2] = map(keyShade[i], 0, keyShadeLevel, 0, b);
        }
      }
      break;
    
    case 2:
      for(byte i = 0; i < 6; i++){
        if(keyShade[i] != 0) keyShade[i] --; //restore to original color over time except for kps indicator (not smooth cause running out of code storage)
                                             //should've used 'm' latch to trigger flashing effect and 3 map() func.
        if(i == 1){
          leds[1][0] = maxKps.r;
          leds[1][1] = maxKps.g;
          leds[1][2] = maxKps.b;
        }
        else{
          leds[i][0] = map(keyShade[i], 0, keyShadeLevel, 0, r);
          leds[i][1] = map(keyShade[i], 0, keyShadeLevel, 0, g);
          leds[i][2] = map(keyShade[i], 0, keyShadeLevel, 0, b);
        }
      }
      break;

    case 3:
      for(byte i = 0; i < 6; i++){
        if(keyShade[0] != 0) keyShade[0] --; //restore to original color over time (using only keyShade[0])
        leds[i][0] = map(keyShade[0], 0, keyShadeLevel, 0, r);
        leds[i][1] = map(keyShade[0], 0, keyShadeLevel, 0, g);
        leds[i][2] = map(keyShade[0], 0, keyShadeLevel, 0, b);
      }
      break;
      
  }
  if(latch == 'l'){ //indicate ledMode by K3-K6
    for(byte i = 2; i < 6; i++){
      if( i == (ledMode+2) ){
        leds[i][0] = 255-r;
        leds[i][1] = 255-g;
        leds[i][2] = 255-b;
      }
      else{
        leds[i][0] = 0;
        leds[i][1] = 0;
        leds[i][2] = 0;
      }
    }
  }
  showLed();
  
  if(keyState[num][7] == true || keyState[num][6] == true){
    
    if(latch == 0 && keyState[num][7] == true && keyState[num][6] == true){ //open settings later
      enterSettings = true;
      Serial.print("Latch:");
      Serial.println(latch, DEC);
    }
    
    if(millis() - prevTime > 400){ //long press trigger
      enterSettings = false;
    
      if( (latch == 0 || latch == 'l') && keyState[num][7] == true && keyState[num][6] == true){ //change LED behavior
        disableFuncKeys();
        if(latch == 0){
          latch = 'l';
        }
        ledMode ++;
        ledMode %= 4;
        EEPROM.update(indexAdr+1, ledMode);
        prevTime = millis();
      }
        
      else if( (latch == 0 || latch == 's') && keyState[num][7] == true){ // swap keymap
        if(latch == 0){
          latch = 's';
        }
        restoreFuncKeys();
        slot++;
        slot %= slotCount;
        backupFuncKeys();
        disableFuncKeys();
        gradient[0].r = initGradient[slot].r;
        gradient[0].g = initGradient[slot].g;
        gradient[0].b = initGradient[slot].b;
        //maxKps.init = true; //de-rem it if you like it
        keyState[num][6] = false; //to not trigger reset max kps
        prevTime = millis();
      }
      
      else if( (latch == 0 || latch == 'm') && keyState[num][6] == true){ // reset max kps
        if(latch == 0){
          backupFuncKeys();
          disableFuncKeys();
          latch = 'm';
          maxKps.init = true;
        }
        prevTime = millis();
      }
      
    }
  }
  else{
    if(latch){
      restoreFuncKeys();
      keyState[num][6] = false;
      keyState[num][7] = false;
      latch = 0;
    }
    Serial.println(enterSettings, BIN);
    if(enterSettings){ //open settings
      settings();
      keyState[num][6] = false;
      keyState[num][7] = false;
      enterSettings = false;
    }
    prevTime = millis();
  }
  //delay(1000);
}

void backupFuncKeys(){
  realKeyValue[0] = key[slot][6];
  realModValue[0] = mod[slot][6];
  realKeyValue[1] = key[slot][7];
  realModValue[1] = mod[slot][7];
}

void disableFuncKeys(){
  key[slot][6] = 0;
  mod[slot][6] = 0;
  key[slot][7] = 0;
  mod[slot][7] = 0;
}

void restoreFuncKeys(){
  key[slot][6] = realKeyValue[0];
  mod[slot][6] = realModValue[0];
  key[slot][7] = realKeyValue[1];
  mod[slot][7] = realModValue[1];
}

void typemsg(){
  Keyboard.packet(KEY_MOD_LSHIFT, charToKey(':'), charToKey(' '));
  Keyboard.packet();
  typestr();
  Keyboard.packet(0,KEY_ENTER);
  Keyboard.packet();
}

boolean hangulLatch = false;

void typestr(){
  byte stringPointer = 0;
  boolean statusFlag = false;
  byte initMod;
  byte num = 0;
  while(1){ //1 break
    
    while(1){ //phrase cutter, 1 break
      
      if(turboMode){ //turboMode - ignore repeating keys in a row
        initMod = modCheck(strBuffer[num+stringPointer]);
        if(
          initMod != modCheck(strBuffer[num+stringPointer+1]) || //The next char's mod is different
          strBuffer[num+stringPointer+1] == 0 || //The next char is /0
          strBuffer[num+stringPointer] == 0 || //Current char is /0
          stringPointer == typeSpeed || //stringPointer has reached the typeSpeed
          (byte)strBuffer[num+stringPointer+1] > B11100000 || //The next char is a 3 byte utf8 character (hangul char)
          (byte)strBuffer[num+stringPointer] > B11100000 //Current char is a 3 byte utf8 character (hangul char)
        ){
          statusFlag = true;
        }
      }
      else{ //normal mode - check repeating keys in a row
        initMod = modCheck(strBuffer[num+stringPointer]);
        if(
          initMod != modCheck(strBuffer[num+stringPointer+1]) || //The next char's mod is different
          strBuffer[num+stringPointer+1] == 0 || //The next char is /0
          strBuffer[num+stringPointer] == 0 || //Current char is /0
          stringPointer == typeSpeed || //stringPointer has reached the typeSpeed
          strBuffer[num+stringPointer] == strBuffer[num+stringPointer+1] || //The next char is same as current
          (byte)strBuffer[num+stringPointer+1] > B11100000 || //The next char is a 3 byte utf8 character (hangul char)
          (byte)strBuffer[num+stringPointer] > B11100000 //Current char is a 3 byte utf8 character (hangul char)
        ){
          statusFlag = true;
        }
      }
      
      if(statusFlag) break;
      stringPointer++;
      
    }
    
    statusFlag = false;
    if(strBuffer[num] == 0) break;
    
    if((byte)strBuffer[num] > B11100000){ //Hangul mode
      if(!hangulLatch){
        Keyboard.packet(KEY_MOD_RALT);
        hangulLatch = true;
      }
      typeKor(num);
      num += 3;
    }
    else{ //Everything else
      if(hangulLatch){
        Keyboard.packet(KEY_MOD_RALT);
        hangulLatch = false;
      }
      Keyboard.packet(
        initMod,
        charToKey(strBuffer[num]),
        stringPointer > 0 ? charToKey(strBuffer[num+1]) : 0,
        stringPointer > 1 ? charToKey(strBuffer[num+2]) : 0,
        stringPointer > 2 ? charToKey(strBuffer[num+3]) : 0,
        stringPointer > 3 ? charToKey(strBuffer[num+4]) : 0,
        stringPointer > 4 ? charToKey(strBuffer[num+5]) : 0,
        stringPointer > 5 ? charToKey(strBuffer[num+6]) : 0,
        stringPointer > 6 ? charToKey(strBuffer[num+7]) : 0,
        stringPointer > 7 ? charToKey(strBuffer[num+8]) : 0,
        stringPointer > 8 ? charToKey(strBuffer[num+9]) : 0,
        stringPointer > 9 ? charToKey(strBuffer[num+10]) : 0,
        stringPointer > 10 ? charToKey(strBuffer[num+11]) : 0
      );
      //From keystroke 2, it checks whether it's out of stringPointer's range using ? : operator.
      //
      //ext.note:
      //These keystrokes are being sent to host as simultaneous keystrokes, so some program like cmd shell would buggy when same keys sent multiple times in a row.
      //So I made turboMode bool to use with these programs.
      num += stringPointer + 1;
      stringPointer = 0;
      Keyboard.packet();
    }
    
  }
  num = 0;
}

void typeKor(byte num){
  unsigned int unichar = 0;
  byte utfChar[3] = {0,};
  
  utfChar[0] |= strBuffer[num+0];
  utfChar[1] |= strBuffer[num+1];
  utfChar[2] |= strBuffer[num+2];

  utfChar[0] &= B00001111;
  utfChar[1] &= B00111111;
  utfChar[2] &= B00111111;
  
  unichar += utfChar[0];
  unichar <<= 6;
  unichar += utfChar[1];
  unichar <<= 6;
  unichar += utfChar[2];

  if(unichar < 0xAC00){ //Hangul jamo
    
    unichar -= 0x3131;
    char jamo[51][3] = {"r", "R", "rt", "s", "sw", "sg", "e", "E", "f", "fr", "fa", "fq", "ft", "fx", "fv", "fg", "a", "q", "Q", "qt", "t", "T", "d", "w", "W", "c", "z", "x", "v", "g", "k", "o", "i", "O", "j", "p", "u", "P", "h", "hk", "ho", "hl", "y", "n", "nj", "np", "nl", "b", "m", "ml", "l"};
    //Hangul jamo set    ㄱ   ㄲ   ㄳ    ㄴ   ㄵ    ㄶ    ㄷ   ㄸ   ㄹ   ㄺ    ㄻ    ㄼ    ㄽ    ㄾ    ㄿ    ㅀ    ㅁ   ㅂ   ㅃ   ㅄ    ㅅ   ㅆ   ㅇ   ㅈ   ㅉ   ㅊ   ㅋ   ㅌ   ㅍ   ㅎ   ㅏ   ㅐ   ㅑ   ㅒ   ㅓ   ㅔ   ㅕ   ㅖ   ㅗ   ㅘ    ㅙ    ㅚ    ㅛ   ㅜ   ㅝ    ㅞ    ㅟ    ㅠ   ㅡ   ㅢ    ㅣ  

    for(byte i = 0; i < strlen(jamo[unichar]); i ++){
      Keyboard.packet( modCheck(jamo[unichar][i]), charToKey(jamo[unichar][i]) );
      Keyboard.packet();
    }
    
  }
  else{
    
    unichar -= 0xAC00;

    //showunichar(unichar);
    
    char chosung[19][2] = {"r", "R", "s", "e", "E", "f", "a", "q", "Q", "t", "T", "d", "w", "W", "c", "z", "x", "v", "g"};
    //Hangul chosung set    ㄱ   ㄲ   ㄴ   ㄷ   ㄸ   ㄹ   ㅁ   ㅂ   ㅃ   ㅅ   ㅆ   ㅇ   ㅈ   ㅉ   ㅊ   ㅋ   ㅌ   ㅍ   ㅎ 
    char jungsung[21][3] = {"k", "o", "i", "O", "j", "p", "u", "P", "h", "hk", "ho", "hl", "y", "n", "nj", "np", "nl", "b", "m", "ml", "l"};
    //Hangul jungsung set    ㅏ   ㅐ   ㅑ   ㅒ   ㅓ   ㅔ   ㅕ   ㅖ   ㅗ   ㅘ    ㅙ    ㅚ    ㅛ   ㅜ   ㅝ    ㅞ    ㅟ    ㅠ   ㅡ   ㅢ    ㅣ
    char jongsung[28][3] = {"", "r", "R", "rt", "s", "sw", "sg", "e", "f", "fr", "fa", "fq", "ft", "fx", "fv", "fg", "a", "q", "qt", "t", "T", "d", "w", "c", "z", "x", "v", "g"};
    //Hangul jongsung set (null) ㄱ   ㄲ   ㄳ    ㄴ   ㄵ    ㄶ    ㄷ   ㄹ   ㄺ    ㄻ    ㄼ    ㄽ    ㄾ    ㄿ    ㅀ    ㅁ   ㅂ   ㅄ    ㅅ   ㅆ   ㅇ   ㅈ   ㅊ   ㅋ   ㅌ   ㅍ   ㅎ

    /*
    chosung = unichar / 588;
    jungsung = unichar / 28;
    jongsung = unichar % 28;
    */

    //Chosung
    Keyboard.packet( modCheck(chosung[unichar/588][0]), charToKey(chosung[unichar/588][0]) );
    Keyboard.packet();

    //Jungsung
    unichar %= 588;
    for(byte i = 0; i < strlen(jungsung[unichar/28]); i ++){
      Keyboard.packet( modCheck(jungsung[unichar/28][i]), charToKey(jungsung[unichar/28][i]) );
      Keyboard.packet();
    }

    //Jongsung
    unichar %= 28;
    for(byte i = 0; i < strlen(jongsung[unichar/1]); i ++){
      Keyboard.packet( modCheck(jongsung[unichar/1][i]), charToKey(jongsung[unichar/1][i]) );
      Keyboard.packet();
    }
    
  }
  Keyboard.packet(0, KEY_SPACE);
  Keyboard.packet();
  Keyboard.packet(0, KEY_BACKSPACE);
  Keyboard.packet();
}

/*
void showunichar(unsigned int unichar){
  for(byte j = 0; j < 2; j ++){
    for(byte i = 0; i < 8; i ++){
      Serial.print(bitRead(unichar, 15-(8*j+i) ), DEC);
    }
    Serial.print(" ");
  }
  Serial.println("UNICHAR");
}
*/

byte modCheck(char charInput){
  byte const shiftMod = KEY_MOD_LSHIFT;
  byte const haneng = KEY_MOD_RALT;
  if(charInput == 58 || charInput == 60 || charInput == 94 || charInput == 95) return (spaceMod = shiftMod);
  else if(33 <= charInput && charInput <= 38) return (spaceMod = shiftMod);
  else if(40 <= charInput && charInput <= 43) return (spaceMod = shiftMod);
  else if(62 <= charInput && charInput <= 90) return (spaceMod = shiftMod);
  else if(123 <= charInput && charInput <= 126) return (spaceMod = shiftMod);
  else if(charInput == ' ') return spaceMod; //allow shift+space for better performance
  else if(charInput == '') return haneng; //for han/eng toggle
  else return (spaceMod = 0);
  //Some words about "return variable = value" expression:
  //It uses = operator's characteristic from standard C language expression.
  //From C11 Standard, section 6.5.16 : "An assignment expression has the value of the left operand after the assignment"
  //So it first assign the value to the variable, THEN it returns the variable, which is same as the value.
  //Be sure to isolate it with (), sometimes it causes bug.
}

byte charToKey(char input){
  switch(input){
    case '`':
    case '~':
      return KEY_GRAVE;
      break;
    case '1':
    case '!':
      return KEY_1;
      break;
    case '2':
    case '@':
      return KEY_2;
      break;
    case '3':
    case '#':
      return KEY_3;
      break;
    case '4':
    case '$':
      return KEY_4;
      break;
    case '5':
    case '%':
      return KEY_5;
      break;
    case '6':
    case '^':
      return KEY_6;
      break;
    case '7':
    case '&':
      return KEY_7;
      break;
    case '8':
    case '*':
      return KEY_8;
      break;
    case '9':
    case '(':
      return KEY_9;
      break;
    case '0':
    case ')':
      return KEY_0;
      break;
    case '-':
    case '_':
      return KEY_MINUS;
      break;
    case '=':
    case '+':
      return KEY_EQUAL;
      break;
    case 9: //tab char, but arduino IDE will not allow full tab character sadly :(
      return KEY_TAB;
      break;
    case 'a':
    case 'A':
      return KEY_A;
      break;
    case 'b':
    case 'B':
      return KEY_B;
      break;
    case 'c':
    case 'C':
      return KEY_C;
      break;
    case 'd':
    case 'D':
      return KEY_D;
      break;
    case 'e':
    case 'E':
      return KEY_E;
      break;
    case 'f':
    case 'F':
      return KEY_F;
      break;
    case 'g':
    case 'G':
      return KEY_G;
      break;
    case 'h':
    case 'H':
      return KEY_H;
      break;
    case 'i':
    case 'I':
      return KEY_I;
      break;
    case 'j':
    case 'J':
      return KEY_J;
      break;
    case 'k':
    case 'K':
      return KEY_K;
      break;
    case 'l':
    case 'L':
      return KEY_L;
      break;
    case 'm':
    case 'M':
      return KEY_M;
      break;
    case 'n':
    case 'N':
      return KEY_N;
      break;
    case 'o':
    case 'O':
      return KEY_O;
      break;
    case 'p':
    case 'P':
      return KEY_P;
      break;
    case 'q':
    case 'Q':
      return KEY_Q;
      break;
    case 'r':
    case 'R':
      return KEY_R;
      break;
    case 's':
    case 'S':
      return KEY_S;
      break;
    case 't':
    case 'T':
      return KEY_T;
      break;
    case 'u':
    case 'U':
      return KEY_U;
      break;
    case 'v':
    case 'V':
      return KEY_V;
      break;
    case 'w':
    case 'W':
      return KEY_W;
      break;
    case 'x':
    case 'X':
      return KEY_X;
      break;
    case 'y':
    case 'Y':
      return KEY_Y;
      break;
    case 'z':
    case 'Z':
      return KEY_Z;
      break;
    case '[':
    case '{':
      return KEY_LEFTBRACE;
      break;
    case ']':
    case '}':
      return KEY_RIGHTBRACE;
      break;
    case '\\':
    case '|':
      return KEY_BACKSLASH;
      break;
    case ';':
    case ':':
      return KEY_SEMICOLON;
      break;
    case '\'':
    case '"':
      return KEY_APOSTROPHE;
      break;
    case ',':
    case '<':
      return KEY_COMMA;
      break;
    case '.':
    case '>':
      return KEY_DOT;
      break;
    case '/':
    case '?':
      return KEY_SLASH;
      break;
    case ' ':
      return KEY_SPACE;
      break;
    case '': //simulate line break
      return KEY_ENTER;
      break;
    case '': //First I thought it must be pretty complicated, but what it turns out was just left alt mod..as I can see on my keyboard. I tried hard getting wrong value all the time!!
      if(constTurboMode) turboMode = !turboMode; //like ㄱ in 학교, the Korean IME won't allow the repeating chars pressed simultaneously.
      return 0;
      break;
    default: //unknown char will displayed to "`"
      return KEY_GRAVE;
      break;
  }
}

void keyToStr(byte keyNum){ //stringlength 지우기
  switch(keyNum){
    case 0:
      str("NONE");
      break;
    case 4:
      str("A");
      break;
    case 5:
      str("B");
      break;
    case 6:
      str("C");
      break;
    case 7:
      str("D");
      break;
    case 8:
      str("E");
      break;
    case 9:
      str("F");
      break;
    case 10:
      str("G");
      break;
    case 11:
      str("H");
      break;
    case 12:
      str("I");
      break;
    case 13:
      str("J");
      break;
    case 14:
      str("K");
      break;
    case 15:
      str("L");
      break;
    case 16:
      str("M");
      break;
    case 17:
      str("N");
      break;
    case 18:
      str("O");
      break;
    case 19:
      str("P");
      break;
    case 20:
      str("Q");
      break;
    case 21:
      str("R");
      break;
    case 22:
      str("S");
      break;
    case 23:
      str("T");
      break;
    case 24:
      str("U");
      break;
    case 25:
      str("V");
      break;
    case 26:
      str("W");
      break;
    case 27:
      str("X");
      break;
    case 28:
      str("Y");
      break;
    case 29:
      str("Z");
      break;
    case 30:
      str("1");
      break;
    case 31:
      str("2");
      break;
    case 32:
      str("3");
      break;
    case 33:
      str("4");
      break;
    case 34:
      str("5");
      break;
    case 35:
      str("6");
      break;
    case 36:
      str("7");
      break;
    case 37:
      str("8");
      break;
    case 38:
      str("9");
      break;
    case 39:
      str("0");
      break;
    case 40:
      str("ENTER");
      break;
    case 41:
      str("ESC");
      break;
    case 42:
      str("BKSPC");
      break;
    case 43:
      str("TAB");
      break;
    case 44:
      str("SPACE");
      break;
    case 45:
      str("-");
      break;
    case 46:
      str("=");
      break;
    case 47:
      str("[");
      break;
    case 48:
      str("]");
      break;
    case 49:
      str("\\");
      break;
    case 50:
      str("#~");
      break;
    case 51:
      str(";");
      break;
    case 52:
      str("'");
      break;
    case 53:
      str("`");
      break;
    case 54:
      str(",");
      break;
    case 55:
      str(".");
      break;
    case 56:
      str("/");
      break;
    case 57:
      str("CAPSLK");
      break;
    case 58:
      str("F1");
      break;
    case 59:
      str("F2");
      break;
    case 60:
      str("F3");
      break;
    case 61:
      str("F4");
      break;
    case 62:
      str("F5");
      break;
    case 63:
      str("F6");
      break;
    case 64:
      str("F7");
      break;
    case 65:
      str("F8");
      break;
    case 66:
      str("F9");
      break;
    case 67:
      str("F10");
      break;
    case 68:
      str("F11");
      break;
    case 69:
      str("F12");
      break;
    case 70:
      str("PRTSCR");
      break;
    case 71:
      str("SCRLK");
      break;
    case 72:
      str("PAUSE");
      break;
    case 73:
      str("INS");
      break;
    case 74:
      str("HOME");
      break;
    case 75:
      str("PGUP");
      break;
    case 76:
      str("DEL");
      break;
    case 77:
      str("END");
      break;
    case 78:
      str("PGDOWN");
      break;
    case 79:
      str("RIGHT");
      break;
    case 80:
      str("LEFT");
      break;
    case 81:
      str("DOWN");
      break;
    case 82:
      str("UP");
      break;
    case 83:
      str("NUMLK");
      break;
    case 84:
      str("KP_/");
      break;
    case 85:
      str("KP_*");
      break;
    case 86:
      str("KP_-");
      break;
    case 87:
      str("KP_+");
      break;
    case 88:
      str("KP_ENTER");
      break;
    case 89:
      str("KP_1");
      break;
    case 90:
      str("KP_2");
      break;
    case 91:
      str("KP_3");
      break;
    case 92:
      str("KP_4");
      break;
    case 93:
      str("KP_5");
      break;
    case 94:
      str("KP_6");
      break;
    case 95:
      str("KP_7");
      break;
    case 96:
      str("KP_8");
      break;
    case 97:
      str("KP_9");
      break;
    case 98:
      str("KP_0");
      break;
    case 99:
      str("KP_.");
      break;
    default:
      bytetostr(keyNum);
      break;
  }
}

void modToStr(byte modNum){
  if(modNum & B00000001){
    str(" + LCTRL");
  }
  if(modNum & B00000010){
    str(" + LSHIFT");
  }
  if(modNum & B00000100){
    str(" + LALT");
  }
  if(modNum & B00001000){
    str(" + LMETA");
  }
  if(modNum & B00010000){
    str(" + RCTRL");
  }
  if(modNum & B00100000){
    str(" + RSHIFT");
  }
  if(modNum & B01000000){
    str(" + RALT");
  }
  if(modNum & B10000000){
    str(" + RMETA");
  }
}

boolean checkEEPROM(char findThis){
  for(indexAdr = 0; indexAdr < EEPROM.length(); indexAdr++){
    if(EEPROM.read(indexAdr) == findThis){
      dataAdr = indexAdr+2; //One index character and one LED mode data
      return 1;
    }
  }
  return 0;
}

char sbv;
byte menuCount;
boolean nlv;
byte curLevel = 0;
byte nlCount;
byte ptr[10]; //Change this if you want more than 10 menu levels
boolean info;
boolean isMenu;
boolean isRtm;

boolean langEng = true;
boolean langKor = false;

#define rtm isRtm = true; sbv = 1; while(nm())
#define ifo if(checkinfo())
#define nwl(stw) if(checknl()) str(stw); if(isbreak()) break; else if(issub()) while(nm()) //NEED OPTIMIZATON :((
#define emsg(stw) if(langEng) msg(stw)
#define estr(stw) if(langEng) str(stw)
#define enwl(stw) if(langEng && checknl()) str(stw); if(langEng && isbreak()) break; else if(langEng && issub()) while(nm()) //NEED OPTIMIZATON :((
#define kmsg(stw) if(langKor) msg(stw)
#define kstr(stw) if(langKor) str(stw)
#define knwl(stw) if(langKor && checknl()) str(stw); if(langKor && isbreak()) break; else if(langKor && issub()) while(nm()) //NEED OPTIMIZATON :((

//I made them into functions so the compiler could reduce the progmem.
boolean checknl(){
  nlv = nl();
  return nlv;
}

boolean isbreak(){
  if( nlv == true && ( (sbv = sb()) < 0) ) return true;
  return false;
}

boolean issub(){
  if( nlv == true && sbv > 0 ) return true;
  return false;
}

boolean checkinfo(){
  if(info) return true;
  return false;
}
//I made them into functions so the compiler could reduce the progmem.

int fastdelay = 20;
int debounce = 10;
int slowdelay = 400; //time to trigger the fast up func

void settings() {
  byte splash = 120;
  byte settingR = 20;
  byte settingG = 5;
  byte settingB = 100;
  for(byte j = 0; j < splash; j ++){
    for(byte i = 0; i < 6; i ++){
      leds[i][0] = map(j, 0, splash, 255 - settingR, settingR);
      leds[i][1] = map(j, 0, splash, 255 - settingG, settingG);
      leds[i][2] = map(j, 0, splash, 255 - settingB, settingB);
      showLed();
    }
  }
  for(byte i = 0; i < 6; i ++){
    leds[i][0] = settingR;
    leds[i][1] = settingG;
    leds[i][2] = settingB;
    showLed();
  }
  
  rtm{ //================================================================
    ifo emsg("== Settings ==");
    ifo kmsg("== 설정 ==");
    ifo msg("");
    ifo estr(": Max kps : ");
    ifo kstr(": 최고 kps : ");
    ifo bytetostr(maxKps.kps);
    ifo keyenter(1);
    ifo estr(": LED Mode : ");
    ifo kstr(": LED 모드 : ");
    ifo bytetostr(ledMode);
    ifo keyenter(1);
    nwl("K. 한국어")
    {
      langEng = false;
      langKor = true;
    }
    nwl("E. English")
    {
      langEng = true;
      langKor = false;
    }
    nwl("1. RGB Test") //nah..
    {
      ifo msg("K7 : Exit RGB test");
      ifo msg("K8 : Slow control (Hold)");
      ifo msg("");
      const byte indvar = 50;
      boolean changed = true;
      byte tr = settingR;
      byte tg = settingG;
      byte tb = settingB;
      while(!FUNC0INPUT){

        for(byte i = 0; i < 6; i ++){
          leds[i][0] = tr;
          leds[i][1] = tg;
          leds[i][2] = tb;
        }
        
        if(KEY0INPUT && tr != 0){
          tr --;
          changed = true;
          leds[0][0] = 0;
          leds[0][1] = indvar;
          leds[0][2] = indvar;
        }
        if(KEY1INPUT && tr != 255){
          tr ++;
          changed = true;
          leds[1][0] = indvar;
          leds[1][1] = 0;
          leds[1][2] = 0;
        }
        
        if(KEY2INPUT && tg != 0){
          tg --;
          changed = true;
          leds[2][0] = indvar;
          leds[2][1] = 0;
          leds[2][2] = indvar;
        }
        if(KEY3INPUT && tg != 255){
          tg ++;
          changed = true;
          leds[3][0] = 0;
          leds[3][1] = indvar;
          leds[3][2] = 0;
        }
        
        if(KEY4INPUT && tb != 0){
          tb --;
          changed = true;
          leds[4][0] = indvar;
          leds[4][1] = indvar;
          leds[4][2] = 0;
        }
        if(KEY5INPUT && tb != 255){
          tb ++;
          changed = true;
          leds[5][0] = 0;
          leds[5][1] = 0;
          leds[5][2] = indvar;
        }
        
        showLed();

        if(changed){
          str(": >r");
          bytetostr(tr);
          str(" g");
          bytetostr(tg);
          str(" b");
          bytetostr(tb);
          keyenter(1);
          changed = false;
          if(FUNC1INPUT){
            delay(100);
          }
        }
        else{
          delayMicroseconds(50);
        }
        
      }
      while(FUNC0INPUT);
    }
    nwl("2. Keymap")
    {
      ifo msg("=== Keymap & slots ===");
      ifo str(": Slots : ");
      ifo bytetostr(slotCount);
      ifo str(" / ");
      ifo bytetostr(slotLimit);
      ifo keyenter(1);
      ifo msg("");
      nwl("1. View current keymap")
      {
        msg("==== current keymap ====");
        msg("");
        showkeymapping();
        for(byte i = 0; i < slotCount; i ++){
          showslot(i);
        }
      }
      nwl("2. Edit a keymap"){ //interrupt 이용 뒤로가기 구현
        msg("==== Keymap edit ====");
        msg("Choose a slot to edit");
        keyenter(1);
        
        str(":>Slot ");
        byte userslot = getnum(0, slotCount - 1);
        str(":");
        
        keyenter(2);
        showkeymapping();
        showslot(userslot);
        msg("");
        for(byte i = 0; i < keyCount; i ++){
          str(":>K");
          bytetostr(i + 1);
          str(" : ");
          key[userslot][i] = getkey(key[userslot][i]);
          mod[userslot][i] = getmod(mod[userslot][i]);
          keyenter(1);
        }
        keyenter(1);
        
        if(askconfirm()){
          writeKeyMod(userslot);
          msg("Complete!");
        }
         
      }
      //nwl("3. Edit slots");
      nwl("3. Swap slots")
      {
        msg("Choose 2 slots to swap");
        msg("");
        str(":>A : Slot ");
        byte a = getnum(0, slotCount - 1);
        keyenter(1);
        str(":>B : Slot ");
        byte b = getnum(0, slotCount - 1);
        keyenter(1);
        msg("");
        if(a == b){
          msg("Error! Choose different slots!");
        }
        else if(askconfirm()){
          for(byte i = 0; i < keyCount; i ++){
            key[a][i] ^= key[b][i];
            key[b][i] ^= key[a][i];
            key[a][i] ^= key[b][i];

            mod[a][i] ^= mod[b][i];
            mod[b][i] ^= mod[a][i];
            mod[a][i] ^= mod[b][i];
          }
          initGradient[a].r ^= initGradient[b].r;
          initGradient[b].r ^= initGradient[a].r;
          initGradient[a].r ^= initGradient[b].r;
          
          initGradient[a].g ^= initGradient[b].g;
          initGradient[b].g ^= initGradient[a].g;
          initGradient[a].g ^= initGradient[b].g;
          
          initGradient[a].b ^= initGradient[b].b;
          initGradient[b].b ^= initGradient[a].b;
          initGradient[a].b ^= initGradient[b].b;

          writeKeyMod(a);
          writeInitGradient(a);
          writeKeyMod(b);
          writeInitGradient(b);

          msg("Complete!");
        }
        
      }
    }
    nwl("3. (in progress)"){
      easteregg();
    }
  }
  
}

void showkeymapping(){
  msg("Keymap :");
  msg("          +----+----+");
  msg(" (Arduino)| K1 | K2 | K7");
  msg("+----+----+----+----+");
  msg("| K3 | K4 | K5 | K6 | K8");
  msg("+----+----+----+----+");
}

boolean askconfirm(){
  str(":>Confirm? : ");
  str("Yes");
  unsigned long presstime;
  while(!KEY5INPUT){
    if(KEY0INPUT || KEY4INPUT){ //up & down
      
      presstime = millis();
      if(strlen(strBuffer) == 3){
        keybackspace(3);
        str("No");
      }
      else{
        keybackspace(2);
        str("Yes");
      }
      
      while(KEY0INPUT || KEY4INPUT){
        if( (millis() - presstime > slowdelay) ){
          if(strlen(strBuffer) == 3){
            keybackspace(3);
            str("No");
          }
          else{
            keybackspace(2);
            str("Yes");
          }
          delay(fastdelay);
        }
      }
      
      delay(debounce);
    }  
  }
  delay(debounce);
  while(KEY5INPUT);
  delay(debounce);
  keyenter(1);
  
  if(strlen(strBuffer) == 3){
    return true;
  }
  else{
    return false;
  }
  
}

void showslot(byte slotnum){
  keyenter(1);
  str(": -- Slot ");
  bytetostr(slotnum);
  str(" --");
  keyenter(1);
  for(byte i = 0; i < 8; i ++){
    str(": K");
    bytetostr(i + 1);
    str(" : ");
    keyToStr(key[slotnum][i]);
    modToStr(mod[slotnum][i]);
    keyenter(1);
  }
}

byte getnum(byte from, byte to){
  byte num = from;
  unsigned long presstime;
  bytetostr(num);
  while(!KEY5INPUT){
    
    if(KEY0INPUT && (num < to) ){ //up
      num ++;
      keybackspace(strlen(strBuffer));
      bytetostr(num);
      presstime = millis();
      while(KEY0INPUT){
        if( (millis() - presstime > slowdelay) && (num < to) ){
          num ++;
          keybackspace(strlen(strBuffer));
          bytetostr(num);
          delay(fastdelay);
        }
      }
      delay(debounce);
    }
    
    if(KEY4INPUT && (num > from) ){ //down
      num --;
      keybackspace(strlen(strBuffer));
      bytetostr(num);
      presstime = millis();
      while(KEY4INPUT){
        if( (millis() - presstime > slowdelay) && (num > from) ){
          num --;
          keybackspace(strlen(strBuffer));
          bytetostr(num);
          delay(fastdelay);
        }
      }
      delay(debounce);
    }
    
  }
  delay(debounce);
  while(KEY5INPUT);
  delay(debounce);
  return num;
}

byte getkey(byte prevkey){
  byte from = 0; //first key
  byte to = 99; //last key
  byte num = prevkey;
  unsigned long presstime;
  keyToStr(num);
  while(!KEY5INPUT){
    
    if(KEY0INPUT && (num < to) ){ //up
      num ++;
      keybackspace(strlen(strBuffer));
      byteToKey(&num);
      presstime = millis();
      while(KEY0INPUT){
        if( (millis() - presstime > slowdelay) && (num < to) ){
          num ++;
          keybackspace(strlen(strBuffer));
          byteToKey(&num);
          delay(fastdelay);
        }
      }
      delay(debounce);
    }
    
    if(KEY4INPUT && (num > from) ){ //down
      num --;
      keybackspace(strlen(strBuffer));
      byteToKey(&num);
      presstime = millis();
      while(KEY4INPUT){
        if( (millis() - presstime > slowdelay) && (num > from) ){
          num --;
          keybackspace(strlen(strBuffer));
          byteToKey(&num);
          delay(fastdelay);
        }
      }
      delay(debounce);
    }
    
  }
  delay(debounce);
  while(KEY5INPUT);
  delay(debounce);
  return num;
}

byte getmod(byte initMod){
  unsigned long presstime;
  byte returnMod = 0;
  char pointer = 0;
  char prevpointer;
  byte previnit = 0;
  boolean stickymod = false;
  
  while(pointer != -1){
    pointer = -1; //default is -1
    
    for(byte i = previnit; i < 8; i ++){ //check for previous mod
      if(bitRead(initMod, i)){
        pointer = i;
        if(!stickymod){
          previnit = i + 1;
        }
        break;
      }
    }
    if(pointer == -1){
      previnit = 8;
    }
    
    bytetomod(pointer); //show current mod
    while(!KEY5INPUT){
      
      if(KEY0INPUT && (pointer < 7) ){ //up
        prevpointer = pointer;
        pointer ++; //move to the next bit (temporary)
        while(bitRead(returnMod, pointer) && (pointer < 8)){ //while the empty bit is found
          pointer ++; //increase pointer
        }
        if(pointer == 8){ //if the empty bit is not found
          pointer = prevpointer; //restore previous value
        }
        //
        if(prevpointer != pointer){ //if it's changed
          keybackspace(strlen(strBuffer));
          bytetomod(pointer);
          presstime = millis();
          
          while(KEY0INPUT){ //while key press
            if( (millis() - presstime > slowdelay) && (pointer < 7) ){ //fast increse
              prevpointer = pointer;
              pointer ++; //move to the next bit (temporary)
              while(bitRead(returnMod, pointer) && (pointer < 8)){ //while the empty bit is found
                pointer ++; //increase pointer
              }
              if(pointer == 8){ //if the empty bit is not found
                pointer = prevpointer; //restore previous value
              }
              //
              if(prevpointer != pointer){ //if it's changed
                keybackspace(strlen(strBuffer));
                bytetomod(pointer);
                delay(fastdelay);
              }
            }
          }
          delay(debounce);
        }
      }
  
      if(KEY4INPUT && (pointer > -1) ){ //down
        prevpointer = pointer;
        pointer --; //move to the prev bit (temporary)
        while(bitRead(returnMod, pointer) && (pointer > -1)){ //while the empty bit is found
          pointer --; //decrease pointer
        }
        //if the empty bit is not found settle down to -1
        //
        if(prevpointer != pointer){ //if it's changed
          keybackspace(strlen(strBuffer));
          bytetomod(pointer);
          presstime = millis();
          
          while(KEY4INPUT){ //while key press
            if( (millis() - presstime > slowdelay) && (pointer > -1) ){ //fast decrease
              prevpointer = pointer;
              pointer --; //move to the previous bit (temporary)
              while(bitRead(returnMod, pointer) && (pointer > -1)){ //while the empty bit is found
                pointer --; //decrease pointer
              }
              //if the empty bit is not found settle down to -1
              //
              if(prevpointer != pointer){ //if it's changed
                keybackspace(strlen(strBuffer));
                bytetomod(pointer);
                delay(fastdelay);
              }
            }
          }
          delay(debounce);
        }
      }
      
    }
    delay(debounce);
    while(KEY5INPUT);
    delay(debounce);
    if(pointer != -1){
      bitWrite(initMod, pointer, false);
      bitWrite(returnMod, pointer, true);
    }
  }
  return returnMod;
}

void byteToKey(byte* num){ //P0iNteR Pr0 (sry)
  if(*num == 1){
    *num = 4; //jump 0 to 4
  }
  else if(*num == 3){
    *num = 0; //jump 4 to 0
  }
  keyToStr(*num);
}

void bytetomod(char num){
  if(num == -1){
    str(" + (END)");
  }
  else{
    modToStr(1 << num);
  }
}

boolean nm(){
  if(sbv != 0){ //new to this menu
    if(isRtm == false) curLevel += sbv;
    else isRtm = false;
    nlCount = 0;
    menuCount = 0;
    if(sbv > 0) ptr[curLevel] = 0;
    sbv = 0;
    info = true;
    isMenu = false;
    return true;
  }
  if(info){
    if(isMenu){
      info = false;
      str("::>");
    }
    else{
      info = false;
      sbv = -1;
      Keyboard.packet(0, KEY_ENTER);
      Keyboard.packet();
      return false;
    }
  }
  return true;
}

boolean nl(){
  if(sbv < 0) return false; //after escapiing from higher level
  if(info){
    str(": ");
    menuCount ++;
    isMenu = true;
    return true;
  }
  if(nlCount == ptr[curLevel]){
    nlCount ++;
    nlCount %= menuCount;
    return true;
  }
  else{
    nlCount ++;
    nlCount %= menuCount;
    return false;
  }
}

char sb(){
  if(info){
    Keyboard.packet(0, KEY_ENTER);
    Keyboard.packet();
    return 0;
  }
  if(nlv){ //found cur menu
    char arrowvalue = 0;
    while(arrowvalue == 0){
      if(KEY5INPUT){ //OK
        arrowvalue = 2;
      }
      if(KEY0INPUT){ //UP
        arrowvalue = -1;
      }
      if(KEY4INPUT){ //DOWN
        arrowvalue = 1;
      }
      if(KEY3INPUT){ //BACK
        arrowvalue = -2;
      }
    }
    delay(5);
    while(KEY5INPUT | KEY0INPUT | KEY4INPUT | KEY3INPUT);
    delay(5);
    if(arrowvalue == -2){ //BACK is the default if multiple keys pressed
      keybackspace(strlen(strBuffer));
      Keyboard.packet(0, KEY_ENTER);
      Keyboard.packet();
      Keyboard.packet(0, KEY_ENTER);
      Keyboard.packet();
      return -1;
    }
    if(arrowvalue == 1 || arrowvalue == -1){
      keybackspace(strlen(strBuffer) + 2); //변경 필요...............................................................
      str(":>");
      ptr[curLevel] += menuCount;
      ptr[curLevel] += arrowvalue;
      ptr[curLevel] %= menuCount;
      return 0;
    }
    if(arrowvalue == 2){
      Keyboard.packet(0, KEY_ENTER);
      Keyboard.packet();
      Keyboard.packet(0, KEY_ENTER);
      Keyboard.packet();
      return 1;
    }
  }
}

void keybackspace(byte count){ //////////////////////////////////////////////////NEED OPTIMIZATION
  for(byte i = 0; i < count; i++){
    Keyboard.packet(0,KEY_BACKSPACE);
    Keyboard.packet();
  }
}

void keyenter(byte count){ //////////////////////////////////////////////////NEED OPTIMIZATION
  for(byte i = 0; i < count; i++){
    Keyboard.packet(0,KEY_ENTER);
    Keyboard.packet();
  }
}

void bytetostr(byte num){
  
  byte reversedigit[3];
  char numcharstr[2] = {0,};
  boolean blankdigit = true;
  
  reversedigit[0] = num / 100;
  reversedigit[1] = (num % 100) / 10;
  reversedigit[2] = num % 10;
  
  for(byte i = 0; i < 3; i ++){
    numcharstr[0] = '0' + reversedigit[i];
    if(numcharstr[0] == '0' && i < 2 && blankdigit == true){
      numcharstr[0] = 0;
    }
    else{
      blankdigit = false;
    }
    strcpy(strBuffer, numcharstr);
    typestr();
  }
  
}

#define NOP __asm__ __volatile__ ("nop\n\t");
#define TGLEDPIN PORTB ^= B01000000 //B를 붙여라!!!!!!!!!

void showLed(){
  noInterrupts();
  for(byte j = 0; j != NUM_LEDS; j++){
    
    (leds[j][1] & B10000000) ? hp() : lp();
    (leds[j][1] & B01000000) ? hp() : lp();
    (leds[j][1] & B00100000) ? hp() : lp();
    (leds[j][1] & B00010000) ? hp() : lp();
    (leds[j][1] & B00001000) ? hp() : lp();
    (leds[j][1] & B00000100) ? hp() : lp();
    (leds[j][1] & B00000010) ? hp() : lp();
    (leds[j][1] & B00000001) ? hp() : lp();
    
    (leds[j][0] & B10000000) ? hp() : lp();
    (leds[j][0] & B01000000) ? hp() : lp();
    (leds[j][0] & B00100000) ? hp() : lp();
    (leds[j][0] & B00010000) ? hp() : lp();
    (leds[j][0] & B00001000) ? hp() : lp();
    (leds[j][0] & B00000100) ? hp() : lp();
    (leds[j][0] & B00000010) ? hp() : lp();
    (leds[j][0] & B00000001) ? hp() : lp();

    (leds[j][2] & B10000000) ? hp() : lp();
    (leds[j][2] & B01000000) ? hp() : lp();
    (leds[j][2] & B00100000) ? hp() : lp();
    (leds[j][2] & B00010000) ? hp() : lp();
    (leds[j][2] & B00001000) ? hp() : lp();
    (leds[j][2] & B00000100) ? hp() : lp();
    (leds[j][2] & B00000010) ? hp() : lp();
    (leds[j][2] & B00000001) ? hp() : lp();
    
  }
  interrupts();
}

void hp(){
  TGLEDPIN;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  TGLEDPIN;
}

void lp(){
  TGLEDPIN;
  NOP;
  NOP;
  NOP;
  TGLEDPIN;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
}

#include <Math.h>

void easteregg(){
  float angle[3] = {0.0, (2.0/3.0)*PI, (2.0/3.0)*PI*2};
  float depth[3]; //z coordinate
  float posit[3]; //x coordinate
  float deltaAngle = 0.0;
  const byte bufsize = 24+1;
  char charbuf[bufsize];
  const PROGMEM char shade[8][6] = {":", "~", "~~", "*~", "**~", "#*~", "#**", "##*"};
  /*const PROGMEM byte chars[30][5] ={ 
                                      b01110000,
                                      b10001000,
                                      b11111000,
                                      b10001000,
                                      b10001000,

                                      b11110000,
                                      b10001000,
                                      b11110000,
                                      b
  
  */
  charbuf[bufsize - 1] = 0;

  while(!KEY3INPUT){
    
    for(byte i = 0; i < bufsize - 1; i ++){
      charbuf[i] = ' ';
    }

    //
    
    depth[0] = sin(angle[0]);
    depth[1] = sin(angle[1]);
    depth[2] = sin(angle[2]);

    depth[0] *= 4.0;
    depth[1] *= 4.0;
    depth[2] *= 4.0;

    depth[0] += 4.0;
    depth[1] += 4.0;
    depth[2] += 4.0;

    //
    
    posit[0] = cos(angle[0]);
    posit[1] = cos(angle[1]);
    posit[2] = cos(angle[2]);
    
    posit[0] *= 7.0;
    posit[1] *= 7.0;
    posit[2] *= 7.0;
    
    posit[0] += 12.5; //반올림 포함
    posit[1] += 12.5;
    posit[2] += 12.5;

    //
    
    float maxvalue;
    byte maxindex[3];

    maxvalue = 0.0;
    for(byte i = 0; i < 3; i ++){
      if(depth[i] > maxvalue){
        maxvalue = depth[i];
        maxindex[2] = i;
      }
    }
    
    maxvalue = 0.0;
    for(byte i = 0; i < 3; i ++){
      if( (i != maxindex[2]) && (depth[i] > maxvalue) ){
        maxvalue = depth[i];
        maxindex[1] = i;
      }
    }

    for(byte i = 0; i < 3; i ++){
      if( (i != maxindex[2]) && (i != maxindex[1]) ){
        maxindex[0] = i;
      }
    }

    //

    for(byte j = 0; j < 3; j ++){
      byte bright = (byte)depth[maxindex[j]];
      byte seat = (byte)posit[maxindex[j]];
      if(bright == 8){
        bright = 7;
      }
      
      for(byte i = 0; i < strlen(shade[bright]); i ++){
        charbuf[seat+i] = shade[bright][i];
        charbuf[seat-i] = shade[bright][i];
      }
    }
  
    strcpy(strBuffer, charbuf);
    typemsg();
    
    float delta = (abs(deltaAngle - PI) - PI/2) * 0.25;
    angle[0] += delta;
    angle[1] += delta;
    angle[2] += delta;
    deltaAngle += 0.04;

    if(deltaAngle > 2.0*PI){
      deltaAngle -= 2.0*PI;
    }
    if(angle[0] > 2.0*PI){
      angle[0] -= 2.0*PI;
    }
    if(angle[1] > 2.0*PI){
      angle[1] -= 2.0*PI;
    }
    if(angle[2] > 2.0*PI){
      angle[2] -= 2.0*PI;
    }

    if(deltaAngle < 0.0){
      deltaAngle += 2.0*PI;
    }
    if(angle[0] < 0.0){
      angle[0] += 2.0*PI;
    }
    if(angle[1] < 0.0){
      angle[1] += 2.0*PI;
    }
    if(angle[2] < 0.0){
      angle[2] += 2.0*PI;
    }

  }
  delay(100);
  while(KEY3INPUT);
  delay(100);
  
}
