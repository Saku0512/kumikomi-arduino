#include "DynamicAuth.h"
#include <Arduino.h>
#include <LiquidCrystal.h>

// --- ピン定義 ---
LiquidCrystal lcd(9, 10, 12, 13, 14, 15);
// const int JoyStick_X = 26;
// const int JoyStick_Y = 27;
const int JoyStick_X = 27;
const int JoyStick_Y = 26;
const int JoyStick_Z = 28; // 押し込みボタン
const int LED = 25;

// --- オブジェクト生成 ---
DynamicAuth auth(JoyStick_X, JoyStick_Y, JoyStick_Z);

// --- 変数 ---
// 右入力計測用変数
unsigned long rightStartTime = 0;
bool isRightActive = false;

// 上入力計測用変数
unsigned long upStartTime = 0;
bool isUpActive = false;

// パスワード設定済みフラグ
bool passwordSet = false;

// 下入力計測用変数
unsigned long downStartTime = 0;
bool isDownActive = false;

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
  if (!isRightActive && !isUpActive &&
      !isDownActive) { // 入力中も表示更新を止める
    lcd.setCursor(0, 0);
    if (!passwordSet) {
      lcd.print("Right 3s: Set   "); // 未設定時のみ表示
    } else {
      lcd.print("                ");
    }
    lcd.setCursor(0, 1);
    lcd.print("Up 3s: Unlock   ");
  }

  Direction dir = auth.getCurrentDirection();

  // --- 1. スティック右長押し処理 (パスワード設定) ---
  if (dir == DIR_RIGHT && !passwordSet) {
    if (!isRightActive) {
      // 倒し始めた瞬間
      isRightActive = true;
      rightStartTime = millis();
    } else {
      // 倒し続けている間
      unsigned long rightDuration = millis() - rightStartTime;

      // カウントダウン表示
      lcd.setCursor(0, 0);
      lcd.print("Hold RIGHT: ");
      lcd.print((3000 - rightDuration) / 1000 + 1);
      lcd.print("s ");

      // 3秒経過判定
      if (rightDuration >= 3000) {
        lcd.clear();
        lcd.print("Release Stick!");

        // スティックが中心に戻るまで待機
        while (auth.getCurrentDirection() != DIR_CENTER) {
          delay(50);
        }
        delay(500);

        // パスワード設定実行
        auth.setPassword(lcd);
        passwordSet = true; // 設定完了フラグ

        // 処理終了後リセット
        isRightActive = false;
        lcd.clear();
      }
    }
  } else {
    isRightActive = false;
  }

  // --- 2.　スティック上長押し処理 (認証開始) ---
  if (dir == DIR_UP) {
    if (!isUpActive) {
      // 倒し始めた瞬間
      isUpActive = true;
      upStartTime = millis();
    } else {
      // 倒し続けている間
      unsigned long upDuration = millis() - upStartTime;

      // カウントダウン表示
      lcd.setCursor(0, 1);
      lcd.print("Hold UP: ");
      lcd.print((3000 - upDuration) / 1000 + 1);
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
          // ロック解除後、再度パスワード設定を可能にする
          passwordSet = false;
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

  if (dir == DIR_DOWN) {
    if (!isDownActive) {
      isDownActive = true;
      downStartTime = millis();
    } else {
      unsigned long downDuration = millis() - downStartTime;

      lcd.setCursor(0, 1);
      lcd.print("DEBUG RESET ");
      lcd.print((3000 - downDuration) / 1000 + 1);
      lcd.print("s ");

      if (downDuration >= 3000) {
        lcd.clear();
        lcd.print("Release to Reset");

        while (auth.getCurrentDirection() != DIR_CENTER) {
          delay(50);
        }
        delay(500);

        // パスワード設定可能状態にリセット
        passwordSet = false;
        lcd.clear();
        lcd.print("!! RESET !!");
        delay(1500);

        lcd.clear();
        isDownActive = false;
      }
    }
  } else {
    isDownActive = false;
  }

  delay(100);
}