#include "DynamicAuth.h"

DynamicAuth::DynamicAuth(int pinX, int pinY, int pinZ) {
    _pinX = pinX;
    _pinY = pinY;
    _pinZ = pinZ;
    _offsetX = 512;
    _offsetY = 512;
    
    // デフォルトパスワード（上上上上）
    for(int i=0; i<PASSWORD_LENGTH; i++) {
        _secretCode[i] = DIR_UP;
    }
}

void DynamicAuth::begin() {
    pinMode(_pinZ, INPUT_PULLUP);
    calibrate();
    Serial.println("Joystick Auth System Initialized");
}

void DynamicAuth::calibrate() {
    // 起動時の位置を中心とする
    _offsetX = analogRead(_pinX);
    _offsetY = analogRead(_pinY);
}

String DynamicAuth::dirToString(Direction dir) {
    switch(dir) {
        case DIR_UP:    return "UP";
        case DIR_DOWN:  return "DOWN";
        case DIR_LEFT:  return "LEFT";
        case DIR_RIGHT: return "RIGHT";
        default:        return "?";
    }
}

// 入力を待つ関数
// 誤入力防止のため「一度中心に戻る」のを待ってから次の入力を受け付けます
Direction DynamicAuth::waitForInput(LiquidCrystal &lcd) {
    Direction detected = DIR_NONE;
    
    // 閾値
    const int THRESHOLD = 200; 

    while (true) {
        int x = analogRead(_pinX) - _offsetX;
        int y = analogRead(_pinY) - _offsetY;

        lcd.setCursor(0, 1);
        lcd.print("X: "); lcd.print(x); lcd.print(" Y: "); lcd.print(y); lcd.print("   ");

        // 現在の状態判定
        Direction current = DIR_NONE;
        if (x < -THRESHOLD) current = DIR_LEFT;
        else if (x > THRESHOLD) current = DIR_RIGHT;
        else if (y < -THRESHOLD) current = DIR_DOWN;   // 配線によってUP/DOWNが逆の場合はここを入れ替えてください
        else if (y > THRESHOLD) current = DIR_UP;
        else current = DIR_CENTER;

        // 入力が確定した場合（中心以外に倒された）
        if (detected == DIR_NONE && current != DIR_CENTER) {
            detected = current;
            // ビープ音などを鳴らすならここ
            Serial.print("Input: "); Serial.println(dirToString(detected));
        }

        // 入力後にスティックが中心に戻ったら、その入力を確定として返す
        if (detected != DIR_NONE && current == DIR_CENTER) {
            delay(100); // チャタリング防止
            return detected;
        }

        delay(10);
    }
}

Direction DynamicAuth::getCurrentDirection() {
    int x = analogRead(_pinX) - _offsetX;
    int y = analogRead(_pinY) - _offsetY;
    const int THRESHOLD = 200; 

    if (x < -THRESHOLD) return DIR_LEFT; // ※配線によりLEFT/RIGHT逆ならここを修正
    if (x > THRESHOLD) return DIR_RIGHT;
    if (y < -THRESHOLD) return DIR_DOWN;    // ※配線によりUP/DOWN逆ならここを修正
    if (y > THRESHOLD) return DIR_UP;
    
    return DIR_CENTER;
}

// パスワード設定モード
void DynamicAuth::setPassword(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Set Password:");
    delay(500);
    Serial.println("=== Start Password Setup ===");
    delay(1000);

    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        lcd.setCursor(0, 1);
        lcd.print("Input #"); lcd.print(i + 1);
        lcd.print(" [   ]"); 
        
        // 入力待ち
        Direction input = waitForInput(lcd);
        _secretCode[i] = input;

        // 確認表示
        lcd.setCursor(9, 1);
        lcd.print(dirToString(input).substring(0, 3)); // 先頭3文字表示
        delay(500);
    }

    lcd.clear();
    lcd.print("Password Saved!");
    Serial.println("Password Saved");
    delay(1500);
}

// 認証モード
bool DynamicAuth::authenticate(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Authenticating...");
    Serial.println("!!LOCK!!");
    Serial.println("\n=== START AUTHENTICATION ===");
    delay(1000);
    
    for (int i = 0; i < PASSWORD_LENGTH; i++) {
        lcd.clear();
        lcd.print("Enter Code #"); lcd.print(i + 1);
        
        // 入力待ち
        Direction input = waitForInput(lcd);

        // ※セキュリティのため、入力した方向は画面に表示せず「*」などを出すのが一般的ですが
        // ここでは分かりやすさ優先で、入力完了だけ示します。
        lcd.setCursor(0, 1);
        lcd.print("* ACCEPTED *");
        
        // 即時判定せず、最後まで入力させる（ブルートフォース対策的にはその方が安全）
        if (input != _secretCode[i]) {
            Serial.println("Auth Failed at step " + String(i+1));
            lcd.clear();
            lcd.print("Wrong Input!");
            delay(1500);
            return false; // 間違った時点で終了させる場合
        }
        delay(300);
    }

    Serial.println("=== AUTHENTICATION SUCCESS ===");
    return true;
}