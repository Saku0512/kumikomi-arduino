#include <Arduino.h>
#include <LiquidCrystal.h>
#include "DynamicAuth.h"

// --- ピン定義 ---
LiquidCrystal lcd(9, 10, 12, 13, 14, 15);
const int JoyStick_X = 26;
const int JoyStick_Y = 27;
const int JoyStick_Z = 11; // 押し込みボタン
const int LED = 25;

// --- オブジェクト生成 ---
DynamicAuth auth(JoyStick_X, JoyStick_Y, JoyStick_Z);

// --- 変数 ---
unsigned long buttonPressTime = 0;
bool isButtonPressed = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.clear();

  // JoyStick_ZのINPUT_PULLUPはDynamicAuth::begin内でも行われますが念のため
  pinMode(JoyStick_Z, INPUT_PULLUP);

  auth.begin(); // ここでキャリブレーション実行
  
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  // 待機画面の表示更新（毎回clearするとちらつくので、必要な時だけ書き換えるのが理想ですが簡易的に）
  if (!isButtonPressed) {
    lcd.setCursor(0, 0);
    lcd.print("Btn: Set Pwd    ");
    lcd.setCursor(0, 1);
    lcd.print("Hold 2s: Unlock ");
  }

  // ボタンの状態読み取り (LOWが押されている状態)
  bool currentBtnState = (digitalRead(JoyStick_Z) == LOW);

  // --- ボタン押下検知ロジック ---
  if (currentBtnState && !isButtonPressed) {
    // 押された瞬間
    isButtonPressed = true;
    buttonPressTime = millis();
    delay(50); // チャタリング防止
  }
  else if (!currentBtnState && isButtonPressed) {
    // 指が離れた瞬間 -> 押されていた時間を計算
    isButtonPressed = false;
    unsigned long duration = millis() - buttonPressTime;

    if (duration >= 2000) {
      // --- 長押し（2秒以上）: 認証開始 ---
      lcd.clear();
      lcd.print("Auth Mode...");
      delay(500);
      
      if (auth.authenticate(lcd)) {
        // 成功時
        lcd.clear();
        lcd.print("!!UNLOCK!!");
        Serial.println("!!UNLOCK!!");
        digitalWrite(LED, HIGH);
        delay(3000);
        digitalWrite(LED, LOW);
      } else {
        // 失敗時
        lcd.clear();
        lcd.print("Auth Failed...");
        delay(2000);
      }

    } else if (duration > 50) {
      // --- 短押し（50ms〜2秒未満）: パスワード設定 ---
      auth.setPassword(lcd);
    }
    
    lcd.clear(); // 処理が終わったら画面クリア
  }
  
  delay(100);
}