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
    pinMode(_pinRotary, INPUT);
    // Knockピンはアナログ入力なのでpinMode不要の場合が多いが、明示しても良い
    _lpfValue = analogRead(_pinKnock);
}

// 簡易的なLPF (ノイズ除去)
int DynamicAuth::getFilteredSensorValue() {
    int raw = analogRead(_pinKnock);
    // [cite: 16] ノイズ排除の簡易実装 (前回のコードの流用)
    _lpfValue = (_lpfValue * 15 + raw) / 16;
    return _lpfValue;
}

// ノック待ち受け処理 (1回分)
int DynamicAuth::readSingleKnock() {
    int peakValue = 0;
    bool knockStarted = false;
    unsigned long startTime = millis();
    
    // タイムアウト設定 (例えば3秒待つ)
    while (millis() - startTime < 3000) {
        int val = getFilteredSensorValue();

        // 閾値を超えたらノック開始とみなす
        if (!knockStarted && val > 20) { 
            knockStarted = true;
        }

        // ノック中の最大値(ピーク)を取得＝「強さ」とする
        if (knockStarted) {
            if (val > peakValue) peakValue = val;
            
            // 値が下がったらノック終了とみなす
            if (val < 15 && peakValue > 20) {
                delay(100); // チャタリング防止
                return peakValue;
            }
        }
        delay(2);
    }
    return 0; // タイムアウト
}

// [処理3] 固有秘密鍵の登録 (3回3セット平均) 
bool DynamicAuth::registerKnockSequence(LiquidCrystal &lcd) {
    long sumPattern[KNOCK_COUNTS_PER_SET] = {0}; // 合計値保持用

    lcd.clear();
    lcd.print("Start Setup...");
    delay(1000);

    // 3セット繰り返す
    for (int set = 1; set <= KNOCK_SETS; set++) {
        lcd.clear();
        lcd.print("Set: ");
        lcd.print(set);
        lcd.print("/");
        lcd.print(KNOCK_SETS);
        
        // 各セットで3回ノックさせる
        for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
            lcd.setCursor(0, 1);
            lcd.print("Knock: ");
            lcd.print(i + 1);
            
            int strength = readSingleKnock();
            
            if (strength == 0) {
                lcd.clear();
                lcd.print("Time out!");
                delay(1000);
                return false; // 失敗
            }

            // 合計に加算
            sumPattern[i] += strength;
            
            // フィードバック
            lcd.setCursor(10, 1);
            lcd.print("OK!");
            delay(500);
        }
        lcd.clear();
        lcd.print("Set Next...");
        delay(1000);
    }

    // 平均値を計算してリファレンス(固有秘密鍵)とする
    // K_ref = Sum / 3 [cite: 17]
    for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
        _refKnockPattern[i] = sumPattern[i] / KNOCK_SETS;
    }

    lcd.clear();
    lcd.print("Knock Reg OK!");
    delay(1000);
    return true;
}

// [処理4] 可変秘密鍵の登録
void DynamicAuth::registerRotationKey(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Set Dial...");
    delay(500);
    
    // ユーザーが値を決める時間を設ける (ここではボタン押下などで確定させるのが理想だが、簡易的に数秒待つ)
    // 実運用ではJoyStickのボタン押下をトリガーにする設計が良いため、
    // ここでは「現在の値を読み取る」機能に留める。
    
    int val = analogRead(_pinRotary);
    // ノイズ対策で数回読んで平均しても良い
    _refRotationValue = val;
    
    lcd.setCursor(0, 1);
    lcd.print("Value: ");
    lcd.print(_refRotationValue);
    delay(1000);
}

// [処理5] 認証実行
bool DynamicAuth::authenticate(LiquidCrystal &lcd) {
    lcd.clear();
    lcd.print("Authenticating...");
    
    // 1. ノックの照合
    for (int i = 0; i < KNOCK_COUNTS_PER_SET; i++) {
        lcd.setCursor(0, 1);
        lcd.print("Knock input: ");
        lcd.print(i + 1);

        int inputStrength = readSingleKnock();
        int refStrength = _refKnockPattern[i];

        // 許容誤差 ±5% の計算 
        int tolerance = refStrength * KNOCK_TOLERANCE_PERCENT / 100;
        int minVal = refStrength - tolerance;
        int maxVal = refStrength + tolerance;

        // 範囲外なら認証失敗
        if (inputStrength < minVal || inputStrength > maxVal) {
            lcd.clear();
            lcd.print("Knock Fail!");
            lcd.setCursor(0,1);
            lcd.print("Diff: ");
            lcd.print(abs(inputStrength - refStrength));
            delay(2000);
            return false;
        }
    }

    // 2. ローテーションの照合
    int currentRotary = analogRead(_pinRotary);
    // ロータリーも少し誤差を許容しないと人間には合わせられないため、仮に範囲を設ける
    if (abs(currentRotary - _refRotationValue) > 50) { // 閾値は調整が必要
        lcd.clear();
        lcd.print("Dial Fail!");
        delay(2000);
        return false;
    }

    // すべて通過
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