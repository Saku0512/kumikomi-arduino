#include <Arduino.h>
#include <LiquidCrystal.h>
#include "DynamicAuth.h"

// --- ピン定義 ---
// ご使用のボードに合わせてピン番号を確認してください
LiquidCrystal lcd(9, 10, 12, 13, 14, 15);
const int JoyStick_X = 26;
const int JoyStick_Y = 27;
const int JoyStick_Z = 11; // 押し込みボタン
const int knock_pin = 28;
const int rotation_pin = 29;
const int LED = 25;

// --- オブジェクト生成 ---
DynamicAuth auth(knock_pin, rotation_pin);

// --- 定数 ---
const int MIN_PAGE = 1;
const int AUTHENTICATION_PAGE = 2;
const int MAX_PAGE = 3;

// --- グローバル変数 ---
int offset_x = 0; // ジョイスティックの中心値
int old_x = 0;
bool is_knock_setup_done = false;   // ノック設定完了フラグ
bool is_encoder_setup_done = false; // ダイヤル設定完了フラグ

// --- 関数プロトタイプ ---
void Unlock();
void handleMenuAction(int page);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  // ジョイスティックのボタンは通常プルアップが必要です
  pinMode(JoyStick_Z, INPUT_PULLUP); 

  lcd.begin(16, 2);
  lcd.clear();

  auth.begin();

  // ジョイスティックのX軸中心値を取得（キャリブレーション）
  offset_x = analogRead(JoyStick_X);

  lcd.setCursor(0, 0);
  lcd.print("Push JoyStick");
  
  // スタート待ち（ボタンが押されるまで待機）
  // INPUT_PULLUPの場合、押すとLOW(0)になります
  while (digitalRead(JoyStick_Z) == HIGH) { 
     delay(50);
  }
  lcd.clear();
}

void loop() {
  int x = analogRead(JoyStick_X);
  // ボタン状態 (LOWが押されている状態と仮定)
  bool isPressed = (digitalRead(JoyStick_Z) == LOW); 
  
  static int current_page = MIN_PAGE;
  
  // --- ページ切り替え処理 ---
  int diff = x - offset_x;
  // デッドゾーン（中心付近のふらつき無視）を設定
  if (abs(diff) > 200) { 
    lcd.clear();
    // 右に入力
    if (diff > 0 && old_x <= offset_x + 200) { 
      current_page++;
      if (current_page > MAX_PAGE) current_page = MIN_PAGE;
    } 
    // 左に入力
    else if (diff < 0 && old_x >= offset_x - 200) { 
      current_page--;
      if (current_page < MIN_PAGE) current_page = MAX_PAGE;
    }
    delay(300); // 連続切り替え防止
  }
  old_x = x;

  // --- メニュー表示 ---
  lcd.setCursor(0, 0);
  switch (current_page) {
    case 1: 
      lcd.print("1.Set Knock Key ");
      lcd.setCursor(0, 1);
      lcd.print(is_knock_setup_done ? "[Set: OK]" : "[Set: None]");
      break;
    case 2:
      lcd.print("2.Start Auth    ");
      lcd.setCursor(0, 1);
      lcd.print("Push to Unlock  ");
      break;
    case 3:
      lcd.print("3.Set Dial Key  ");
      lcd.setCursor(0, 1);
      lcd.print(is_encoder_setup_done ? "[Set: OK]" : "[Set: None]");
      break;
  }

  // --- 決定ボタン処理 ---
  if (isPressed) { 
    delay(200); // チャタリング防止
    handleMenuAction(current_page);
    lcd.clear(); // 処理が終わったら画面クリア
  }

  delay(100);
}

// メニューの選択に応じた処理を実行
void handleMenuAction(int page) {
  switch (page) {
    case 1: // ノック設定
    Serial.print("!!LOCK!!");
      if (auth.registerKnockSequence(lcd)) {
        is_knock_setup_done = true;
      }
      break;
    
    case 3: // ダイヤル設定
      auth.registerRotationKey(lcd);
      is_encoder_setup_done = true;
      break;

    case 2: // 認証開始
      if (is_knock_setup_done && is_encoder_setup_done) {
        Unlock();
      } else {
        lcd.clear();
        lcd.print("Setup Required!"); // 設定がまだです
        delay(1500);
      }
      break;
  }
}

// 解錠処理
void Unlock() {
  if (auth.authenticate(lcd)) {
    lcd.clear();
    lcd.print("!!UNLOCK!!");
    Serial.print("!!UNLOCK!!");
    digitalWrite(LED, HIGH); // LED点灯
    delay(3000);
    digitalWrite(LED, LOW);
  } else {
    lcd.clear();
    lcd.print("Auth Failed...");
    delay(2000);
  }
}