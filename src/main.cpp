#include <Arduino.h>
#include <LiquidCrystal.h>
#include "DynamicAuth.h"

// --- ピン定義 ---
LiquidCrystal lcd(9, 10, 12, 13, 14, 15);
const int JoyStick_X = 26;
const int JoyStick_Y = 27;
const int JoyStick_Z = 28; // 押し込みボタン
const int LED = 25;

// --- オブジェクト生成 ---
DynamicAuth auth(JoyStick_X, JoyStick_Y, JoyStick_Z);

// --- 変数 ---
unsigned long buttonPressTime = 0;
bool isButtonPressed = false;

// 上入力計測用変数
unsigned long upStartTime = 0;
bool isUpActive = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.clear();

  pinMode(JoyStick_Z, INPUT_PULLUP);

  auth.begin(); 
  
  lcd.print("System Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  // 待機画面の表示更新
  if (!isButtonPressed && !isUpActive) { // 上入力中も表示更新を止める
    lcd.setCursor(0, 0);
    lcd.print("Btn: Set Pwd    ");
    lcd.setCursor(0, 1);
    lcd.print("Up 3s: Unlock   "); // 表示変更
  }

  // --- 1. ボタン処理 (パスワード設定のみに変更) ---
  bool currentBtnState = (digitalRead(JoyStick_Z) == LOW);

  if (currentBtnState && !isButtonPressed) {
    isButtonPressed = true;
    buttonPressTime = millis();
    delay(50); 
  }
  else if (!currentBtnState && isButtonPressed) {
    isButtonPressed = false;
    unsigned long duration = millis() - buttonPressTime;

    // 長押し判定を削除し、短押しで設定のみにする
    if (duration > 50) {
      auth.setPassword(lcd);
    }
    
    lcd.clear(); 
  }

  // --- 2.　スティック上長押し処理 (認証開始) ---
  // libを変更済みとのことなので getCurrentDirection() を使用します
  Direction dir = auth.getCurrentDirection();

  if (dir == DIR_UP) {
    if (!isUpActive) {
      // 倒し始めた瞬間
      isUpActive = true;
      upStartTime = millis();
    } else {
      // 倒し続けている間
      unsigned long upDuration = millis() - upStartTime;

      // カウントダウン表示 (オプション)
      lcd.setCursor(0, 1);
      lcd.print("Hold UP: ");
      lcd.print((3000 - upDuration)/1000 + 1);
      lcd.print("s   ");

      // 3秒経過判定
      if (upDuration >= 3000) {
        lcd.clear();
        lcd.print("Release Stick!"); // 重要: スティックを戻させる指示
        
        // ★重要: スティックが中心に戻るまで待機
        // (これがないと「上」がパスワードの1文字目として勝手に入力されてしまう)
        while (auth.getCurrentDirection() != DIR_CENTER) {
          delay(50);
        }
        delay(500); // 誤操作防止のウェイト

        // 認証実行 (元のコードの処理をここに移動)
        if (auth.authenticate(lcd)) {
          lcd.clear();
          lcd.print("!!UNLOCK!!");
          Serial.println("!!UNLOCK!!");
          digitalWrite(LED, HIGH);
          delay(3000);
          digitalWrite(LED, LOW);
        } else {
          lcd.clear();
          lcd.print("Auth Failed...");
          delay(2000);
        }
         
        // 処理終了後リセット
        isUpActive = false;
        lcd.clear();
      }
    }
  } else {
    // 上以外ならタイマーリセット
    isUpActive = false;
  }
  
  delay(100);
}