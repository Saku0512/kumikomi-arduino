#ifndef DYNAMIC_AUTH_H
#define DYNAMIC_AUTH_H

#include <Arduino.h>
#include <LiquidCrystal.h>

// 仕様書に基づく定数定義
#define KNOCK_TOLERANCE_PERCENT 5  //  振動許容誤差 ±5%
#define KNOCK_SETS 3               // [cite: 17] 平均を取るセット数
#define KNOCK_COUNTS_PER_SET 3     // [cite: 17] 1セットあたりのノック回数

class DynamicAuth {
private:
    int _pinKnock;
    int _pinRotary;
    
    // 登録された「正解」データ (固有秘密鍵)
    int _refKnockPattern[KNOCK_COUNTS_PER_SET]; 
    int _refRotationValue;

    // ノック検出用の内部変数
    int _lpfValue; // Low Pass Filter用

    // 内部関数: 単発のノックを検出してその強さを返す
    int readSingleKnock();
    
    // 内部関数: LPFを通したセンサ値取得
    int getFilteredSensorValue();

public:
    // コンストラクタ
    DynamicAuth(int pinKnock, int pinRotary);

    // 初期化
    void begin();

    // --- 仕様書 5.1 処理フロー ---
    
    // [処理3] 固有秘密鍵の登録 (3セット平均法)
    // 戻り値: 成功ならtrue
    bool registerKnockSequence(LiquidCrystal &lcd);

    // [処理4] 可変秘密鍵の登録 (ロータリーエンコーダ/VR)
    void registerRotationKey(LiquidCrystal &lcd);

    // [処理5] 自動認証実行 (比較)
    // ノックとローテーションの両方が一致するか確認
    bool authenticate(LiquidCrystal &lcd);

    // デバッグ用: 保存された値を表示
    void debugPrintKeys();
};

#endif