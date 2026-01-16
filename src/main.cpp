//ライブラリを読み込む
#include <LiquidCrystal.h>
#include <Arduino.h>

enum {
  knock_authentication,
  encoder_authentication
};
 
int knock_pattern[3][3] = {0};
int lpf = 0;
int offset = 0;

bool setKnock_pattern(); 
bool setEncoder_rotate();
bool saved_data(int authentication, bool isSave);

// //LiquidCrystal型の変数を用意
// LiquidCrystal lcd(9,10,12,13,14,15);
// const int JoyStick_X = 26;
// const int JoyStick_Y = 27;
// const int JoyStick_Z = 11;

//LiquidCrystal型の変数を用意
LiquidCrystal lcd(12,11,5,4,3,2);
const int JoyStick_X = A0;
const int JoyStick_Y = A1;
const int JoyStick_Z = 7;
const int knock = A2;

const int MIN_PAGE = 1;
const int AUTHENTICATION_PAGE = 2;
const int MAX_PAGE = 3;

void setup() {
  Serial.begin(115200);
  
  //LCDの桁数と行数を指定する(16桁2行)
  lcd.begin(16,2);

  //画面をクリアしてカーソルの位置を左上(0,0)にする
  lcd.clear();

  pinMode(JoyStick_Z, INPUT);

  lcd.setCursor(0, 0);

  int x = analogRead(JoyStick_X);
  offset = x;
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
    if (z) knock_result = saved_data(knock_authentication, knock_result);
  } else if (current_page == MAX_PAGE) {
    if (z) encoder_result = saved_data(encoder_authentication, encoder_result);
  } 

  if (knock_result && encoder_result) {
    if (current_page == AUTHENTICATION_PAGE && z) {
      Serial.println("AUTHENTICATION");
    }
  }

  lcd.setCursor(1,1);
  lcd.print(current_page);
   
  // Serial.print(x , DEC);
  // Serial.print(" ");
  // Serial.print(offset, DEC);
  // Serial.print(" ");
  // Serial.println(z, DEC);

  old_x = x;
  old_y = y;

  delay(100);
}

// Knock authentication(setting)
bool setKnock_pattern() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("please knock the");
  lcd.setCursor(0,1);
  lcd.print("sensor");

  int count = 0;                 // 0〜8（合計9回）
  bool knock_active = false;
  unsigned long last_knock = 0;

  // LPF 初期化
  int kn = analogRead(knock);
  lpf = kn;

  while (count < 9) {
    kn = analogRead(knock);
    lpf = (lpf * 15 + kn) / 16;

    // ノック開始（立ち上がり）
    if (!knock_active && lpf > 10 && millis() - last_knock > 200) {
      knock_active = true;
      last_knock = millis();

      int i = count / 3;
      int j = count % 3;
      knock_pattern[i][j] = lpf;
      count++;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Knock ");
      lcd.print(count);
    }

    // ノック終了（戻ったら解除）
    if (knock_active && lpf < 80) {
      knock_active = false;
    }

    delay(5);
  }
  return 1;
}

bool saved_data(int authentication, bool isSave) {
  static bool saved = false;
  switch (authentication) {
    case knock_authentication:
      if (isSave) {
        lcd.clear();
        lcd.print("knock complete!");
        saved = true;
      } else {
        lcd.clear();
        saved = setKnock_pattern();
      }
    break;

    case encoder_authentication:
      if (isSave) {
        lcd.clear();
        lcd.print("rotaly complete!");
        saved = true;
      } else {
        lcd.clear();
        saved = setEncoder_rotate();
      }
    break;
  } 
  return saved;
}

bool setEncoder_rotate() {
  return true; // ダミー
}
