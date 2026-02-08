#include "DynamicAuth.h"

DynamicAuth::DynamicAuth(int pinKnock, int pinRotary) {
    _pinKnock = pinKnock;
    _pinRotary = pinRotary;
    _lpfValue = 0;
    _refRotationValue = 0;
    // 配列初期化
    for(int i=0; i<KNOCK_COUNTS_PER_SET; i++) {
        _refKnockPattern[i] = 0;
    }
}

void DynamicAuth::begin() {
    // pinKnockはアナログ入力なので通常設定不要ですが、明示的に初期値を読みます
    _lpfValue = analogRead(_pinKnock);
    // メイン側で Serial.begin(115200); が呼ばれている前提です
    Serial.println("DynamicAuth System Initialized");
}

// 簡易的なLPF (ノイズ除去)
int DynamicAuth::getFilteredSensorValue() {
    int raw = analogRead(_pinKnock);
    // 前回の値に重み付けをして急激な変化（スパイクノイズ）を抑える
    _lpfValue = (_lpfValue * 15 + raw) / 16;
    return _lpfValue;
}

// ノック1回分の強さを取得
int DynamicAuth::readSingleKnock() {
    int peakValue = 0;
    bool knockStarted = false;
    unsigned long startTime = millis();
    
    Serial.println("--- Waiting for Knock ---");

    // 3秒間待機
    while (millis() - startTime < 3000) {
        int val = getFilteredSensorValue();

        // デバッグ用：値が少しでも動いたら表示（ノイズ確認用）
        // if (val > 5) { Serial.print("."); }

        // 閾値を超えたらノック開始とみなす (センサー感度により 20 を調整してください)
        if (!knockStarted && val > 20) { 
            knockStarted = true;
            Serial.println("\nKnock Detected! Recording peak...");
        }

        // ノック中の最大値(ピーク)を取得
        if (knockStarted) {
            if (val > peakValue) peakValue = val;
            
            // 値が下がったらノック終了とみなす
            if (val < 15 && peakValue > 20) {
                delay(150); // チャタリング（余韻）防止
                Serial.print("Knock Peak Value: ");
                Serial.println(peakValue);
                return peakValue;
            }
        }
        delay(2);
    }
    
    Serial.println("\nKnock Timeout (No input)");
    return 0; // タイムアウト
}

// [処理3] ノックパターンの登録 (3セット平均法)
bool DynamicAuth::registerKnockSequence(LiquidCrystal &lcd) {
    long sumPattern[KNOCK_COUNTS_PER_SET] = {0}; // 合計値保持用

    lcd.clear();
    lcd.print("Start Setup...");
    Serial.println("=== Start Knock Setup ===");
    delay(1000);

    // セット数分繰り返す (例: 3セット)
    for (int set = 1; set <= KNOCK_SETS; set++) {
        lcd.clear();
        lcd.print("Set: "); lcd.print(set); lcd.print("/"); lcd.print(KNOCK_SETS);
        Serial.print("\n--- Set "); Serial.print(set); Serial.println(" ---");
        delay(1000); // 準備時間
        
        // 1セット内の回数分ノックさせる (例: 3回)
        for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
            lcd.setCursor(0, 1);
            lcd.print("Knock #"); lcd.print(i + 1); lcd.print("     "); 
            
            int strength = readSingleKnock();
            
            if (strength == 0) {
                lcd.clear();
                lcd.print("Time out!");
                Serial.println("Setup Failed: Timeout");
                delay(1000);
                return false; // 失敗
            }

            // --- 強化した表示 ---
            // LCDに数値を出す
            lcd.setCursor(10, 1);
            lcd.print(strength);
            lcd.print("   "); // 前の数字を消す空白
            
            // Serialにも出す
            Serial.print("Input "); Serial.print(i+1); 
            Serial.print(": "); Serial.println(strength);
            
            delay(500); 

            // 合計に加算
            sumPattern[i] += strength;
        }
        
        lcd.clear();
        lcd.print("Set "); lcd.print(set); lcd.print(" OK!");
        delay(800);
    }

    // 平均値を計算して保存
    Serial.println("\n=== Calculating Reference Keys ===");
    for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
        _refKnockPattern[i] = sumPattern[i] / KNOCK_SETS;
        
        // 保存された鍵を表示
        Serial.print("Key["); Serial.print(i); Serial.print("]: ");
        Serial.println(_refKnockPattern[i]);
    }

    lcd.clear();
    lcd.print("Knock Saved!");
    delay(1000);
    return true;
}

// [処理4] ダイヤル位置の登録
void DynamicAuth::registerRotationKey(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Set Dial...");
    Serial.println("=== Start Rotary Setup ===");
    
    // 3秒間リアルタイム値を表示して、ユーザーに調整させる
    unsigned long start = millis();
    while(millis() - start < 3000) {
        int val = analogRead(_pinRotary);
        lcd.setCursor(0, 1);
        lcd.print("Val: "); lcd.print(val); lcd.print("   ");
        // Serial.println(val); 
        delay(100);
    }
    
    // 最終値を保存
    _refRotationValue = analogRead(_pinRotary);
    
    Serial.print("Rotary Key Saved: ");
    Serial.println(_refRotationValue);

    lcd.clear();
    lcd.print("Dial Saved:");
    lcd.setCursor(0, 1);
    lcd.print(_refRotationValue);
    delay(1000);
}

// [処理5] 認証実行
bool DynamicAuth::authenticate(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Authenticating...");
    Serial.println("\n=== START AUTHENTICATION ===");
    delay(1000);
    
    // 1. ノックの照合
    for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
        lcd.clear();
        lcd.print("Knock #"); lcd.print(i + 1); lcd.print(" ?");
        
        Serial.print("Waiting for Auth Knock #"); Serial.println(i+1);

        int inputStrength = readSingleKnock();
        int refStrength = _refKnockPattern[i];

        // 許容誤差の計算
        int tolerance = refStrength * KNOCK_TOLERANCE_PERCENT / 100;
        if(tolerance < 10) tolerance = 10; // 最小許容値を設定

        int diff = abs(inputStrength - refStrength);

        // 詳細な判定ログ
        Serial.print("Input: "); Serial.print(inputStrength);
        Serial.print(" / Ref: "); Serial.print(refStrength);
        Serial.print(" (Tol: "); Serial.print(tolerance);
        Serial.print(") -> Diff: "); Serial.println(diff);

        // 判定
        if (diff > tolerance) {
            lcd.clear();
            lcd.print("Knock Fail!");
            lcd.setCursor(0,1);
            lcd.print("In:"); lcd.print(inputStrength);
            lcd.print(" Ref:"); lcd.print(refStrength);
            
            Serial.println("!!! KNOCK AUTH FAILED !!!");
            delay(3000); // 失敗した数値をしっかり見せる
            return false;
        }
        
        lcd.setCursor(12, 0);
        lcd.print("OK");
        Serial.println("-> OK");
        delay(500);
    }

    // 2. ダイヤルの照合
    lcd.clear();
    lcd.print("Check Dial...");
    Serial.println("Checking Rotary...");
    delay(1000); 
    
    int currentRotary = analogRead(_pinRotary);
    int rotaryDiff = abs(currentRotary - _refRotationValue);

    Serial.print("Current: "); Serial.print(currentRotary);
    Serial.print(" / Ref: "); Serial.print(_refRotationValue);
    Serial.print(" -> Diff: "); Serial.println(rotaryDiff);
    
    // ダイヤル誤差許容範囲
    if (rotaryDiff > 100) { 
        lcd.clear();
        lcd.print("Dial Fail!");
        lcd.setCursor(0,1);
        lcd.print(currentRotary);
        lcd.print(" vs ");
        lcd.print(_refRotationValue);
        
        Serial.println("!!! ROTARY AUTH FAILED !!!");
        delay(2000);
        return false;
    }

    Serial.println("=== AUTHENTICATION SUCCESS ===");
    return true;
}

void DynamicAuth::debugPrintKeys() {
    Serial.print("KEY KNOCK: ");
    for(int i=0; i<3; i++) {
        Serial.print(_refKnockPattern[i]);
        Serial.print(" ");
    }
    Serial.print(" | ROTARY: ");
    Serial.println(_refRotationValue);
}