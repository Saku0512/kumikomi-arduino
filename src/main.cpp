//ライブラリを読み込む
#include <LiquidCrystal.h>
#include <Arduino.h>

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
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("please knock the");
  lcd.setCursor(0,1);
  lcd.print("sensor");

  int count = 0;               
  bool knock_active = false;
  unsigned long last_knock = 0;

  // LPF 初期化
  int kn = analogRead(knock);
  lpf = kn;

  while (count < 3) {
    kn = analogRead(knock);
    lpf = (lpf * 15 + kn) / 16;

    int knock_interval = millis() - last_knock;
    // ノック開始（立ち上がり）
    if (!knock_active && lpf > 100 && knock_interval > 1000) {
      knock_active = true;
      last_knock = millis();

      Knock_Data.Strength[count] = lpf;
//      Knock_Data[i].Interval[j] = knock_interval;
      Knock_Data.Strength_sum += Knock_Data.Strength[count];
//      Knock_Data[j].Interval_ave += Knock_Data[i].Interval[j];
      count++;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("knock ");
      lcd.print(count);
      lcd.setCursor(0,1);
      lcd.print("str ");
      lcd.print(Knock_Data.Strength[count]);
    }
    
    // ノック終了（戻ったら解除）
    if (knock_active && lpf < 80) {
      knock_active = false;
    }

    delay(5);
  }
  
  Knock_Data.Strength_ave = (int)(Knock_Data.Strength_sum / 3);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ave ");
  lcd.print(Knock_Data.Strength_ave);
  return 1;
}

bool setEncoder_rotate() {
  const int START_TH = 12;
  const int STOP_TH  = 3;

  unsigned long stop_time = 0;

  int count = 0;
  int last = analogRead(rotation);
  bool rotating = false;

  Rotary.sum = 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("rotate sensor");

  while (count < 3) {
    int now = analogRead(rotation);
    int delta = abs(now - last);

    if (!rotating && delta > START_TH) {
      rotating = true;
      stop_time = millis();
      Rotary.data[count] = 0;   // ★必須
    }

    if (rotating) {
      Rotary.data[count] = max(Rotary.data[count], delta);

      if (delta < STOP_TH) {
        if (millis() - stop_time > 200) {
          rotating = false;
          Rotary.sum += Rotary.data[count];
          count++;

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("rotary ");
          lcd.print(count);
          lcd.setCursor(0, 1);
          lcd.print("data ");
          lcd.print(Rotary.data[count - 1]);

          delay(500);
        }
      } else {
        stop_time = millis();
      }
    }

    last = now;
    delay(500);
  }

  Rotary.ave = Rotary.sum / 3;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("rotary ave");
  lcd.setCursor(0, 1);
  lcd.print(Rotary.ave);

  delay(1000);
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

int measureRotaryOnce(unsigned long timeout_ms = 2000) {
  const int START_TH = 12;
  const int STOP_TH  = 3;

  int last = analogRead(rotation);
  bool rotating = false;
  int max_delta = 0;
  unsigned long stop_time = 0;
  unsigned long rotate_start_time = 0;

  while (true) {
    int now = analogRead(rotation);
    int delta = abs(now - last);

    // 回転開始待ち（ここでは timeout を見ない）
    if (!rotating && delta > START_TH) {
      rotating = true;
      max_delta = 0;
      stop_time = millis();
      rotate_start_time = millis();
    }

    if (rotating) {
      max_delta = max(max_delta, delta);

      // 回転終了判定
      if (delta < STOP_TH) {
        if (millis() - stop_time > 200) {
          return max_delta;   // 正常確定
        }
      } else {
        stop_time = millis();
      }

      // 回転中 timeout
      if (millis() - rotate_start_time > timeout_ms) {
        return 0; // 回転したが異常
      }
    }

    last = now;
    delay(1000);
  }
}

int measureKnockOnce(unsigned long timeout_ms = 2000) {
  unsigned long start = millis();
  int lpf = analogRead(knock);
  int max_lpf = 0;
  bool active = false;

  while (millis() - start < timeout_ms) {
    int kn = analogRead(knock);
    lpf = (lpf * 15 + kn) / 16;

    if (!active && lpf > 100) {
      active = true;
      max_lpf = lpf;
    }

    if (active) {
      max_lpf = max(max_lpf, lpf);
      if (lpf < 80) {
        return max_lpf; // 確定
      }
    }

    delay(5);
  }

  return 0; // タイムアウト
}

void Unlock() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("knock password");

  int knock_val = measureKnockOnce();
  if (knock_val == 0 ||
      abs(knock_val - Knock_Data.Strength_ave) > Knock_Data.Strength_ave * 0.4) {
    lcd.clear();
    lcd.print("knock error");
    delay(800);
    return;
  }

  lcd.clear();
  lcd.print("knock OK");
  delay(500);

  lcd.clear();
  lcd.print("rotary pass");

  int rotary_val = measureRotaryOnce();
  if (rotary_val == 0 ||
      abs(rotary_val - Rotary.ave) > Rotary.ave * 0.2) {
    lcd.clear();
    lcd.print("rotary error");
    delay(800);
    return;
  }

  lcd.clear();
  lcd.print("UNLOCK!");
  digitalWrite(LED, HIGH);
  Serial.print("!! UNLOCK !!");
}
