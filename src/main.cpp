//ライブラリを読み込む
#include <LiquidCrystal.h>
#include <Arduino.h>
#include "DynamicAuth.h"

enum {
  knock_authentication,
  encoder_authentication
};

struct Knock {
  int Strength[3] = {0};
  int Strength_sum = 0;
  int Strength_ave = 0;
};

struct Rotary_data {
  int data[3] = {0};
  int sum = 0;
  int ave = 0;  
};

Rotary_data Rotary;
Knock Knock_Data;

int lpf = 0;
int offset = 0;

bool setKnock_pattern(); 
bool setEncoder_rotate();
bool saved_data(int authentication, bool isSave, int z);
void Unlock();

// //LiquidCrystal型の変数を用意
 LiquidCrystal lcd(9,10,12,13,14,15);
 const int JoyStick_X = 26;
 const int JoyStick_Y = 27;
 const int JoyStick_Z = 11;
 const int knock = 28;
 const int rotation = 29;
 const int LED = 25;

// DynamicAuth オブジェクト初期化
DynamicAuth auth(knock, rotation);

//LiquidCrystal型の変数を用意
//LiquidCrystal lcd(12,11,5,4,3,2);
//const int JoyStick_X = A0;
//const int JoyStick_Y = A1;
//const int JoyStick_Z = 7;
//const int knock = A2;

const int MIN_PAGE = 1;
const int AUTHENTICATION_PAGE = 2;
const int MAX_PAGE = 3;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  //LCDの桁数と行数を指定する(16桁2行)
  lcd.begin(16,2);

  //画面をクリアしてカーソルの位置を左上(0,0)にする
  lcd.clear();

  pinMode(JoyStick_Z, INPUT);

  // DynamicAuth初期化
  auth.begin();

  lcd.setCursor(0, 0);

  int x = analogRead(JoyStick_X);
  offset = x;

  static bool start = false;
  int z = digitalRead(JoyStick_Z);
  while (!z || !start) {
    z = digitalRead(JoyStick_Z);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("push joyStick");
    delay(100);
    if (z) start = true;
  }
}

int old_x = 0;
int old_y = 0;

void loop() {
  // lcd.cursor();
  int x = analogRead(JoyStick_X);
  int y = analogRead(JoyStick_Y);
  int z = digitalRead(JoyStick_Z);

  static int current_page = 0;

  x = (abs(offset - x) < 10) ? -1 : x;
  if (x != -1) {
    lcd.clear();
    if (old_x < x) {          // 右
      current_page++;
      if (current_page > MAX_PAGE) current_page = MIN_PAGE;
    } else if (old_x > x) { // 左
      current_page--; 
      if (current_page < MIN_PAGE) current_page = MAX_PAGE;
    }
    delay(500);
  }

  static bool knock_result = false;
  static bool encoder_result = false;

  if (current_page == MIN_PAGE) {
    knock_result = saved_data(knock_authentication, knock_result, z);
  } else if (current_page == MAX_PAGE) {
    encoder_result = saved_data(encoder_authentication, encoder_result, z);
  } 

  if (knock_result && encoder_result) {
    if (current_page == AUTHENTICATION_PAGE && z) {
      Serial.println("AUTHENTICATION");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AUTHENTICATION");
      Unlock();
    }
  }

  lcd.setCursor(1,1);
  lcd.print(current_page);
  
  Serial.println(analogRead(rotation));

  old_x = x;
  old_y = y;

  delay(100);
}

// Knock authentication(setting)
bool setKnock_pattern() {
  return auth.registerKnockSequence(lcd);
}

bool setEncoder_rotate() {
  auth.registerRotationKey(lcd);
  return true;
}

bool saved_data(int authentication, bool isSave, int z) {
  if (z) isSave = false;
  switch (authentication) {
    case knock_authentication:
      static bool knock_saved = false;
      if (isSave) {
        lcd.clear();
        lcd.print("knock complete!");
        knock_saved = true;
      } else {
        lcd.clear();
        knock_saved = setKnock_pattern();
      }
      return knock_saved;
    break;

    case encoder_authentication:
      static bool encoder_saved = false;
      if (isSave) {
        lcd.clear();
        lcd.print("rotary complete!");
        encoder_saved = true;
      } else {
        lcd.clear();
        if (z) encoder_saved = setEncoder_rotate();
      }
      return encoder_saved;
    break;
  } 
  return false;
}


void Unlock() {
  if (auth.authenticate(lcd)) {
    lcd.clear();
    lcd.print("!!UNLOCK!!");
    digitalWrite(LED, HIGH);
    Serial.print("!!UNLOCK!!");
  } else {
    lcd.clear();
    lcd.print("Authentication Failed");
    delay(800);
    return;
  }
}
