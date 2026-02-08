#ifndef DYNAMIC_AUTH_H
#define DYNAMIC_AUTH_H

#include <Arduino.h>
#include <LiquidCrystal.h>

// 仕様設定
#define KNOCK_TOLERANCE_PERCENT 20  // 許容誤差 ±20% (5%だと人間には厳しすぎるため緩和)
#define KNOCK_SETS 3                // 平均を取るセット数
#define KNOCK_COUNTS_PER_SET 3      // 1セットあたりのノック回数

class DynamicAuth {
private:
    int _pinKnock;
    int _pinRotary;
    
    // 登録された「正解」データ (固有秘密鍵)
    int _refKnockPattern[KNOCK_COUNTS_PER_SET]; 
    int _refRotationValue;

    // 内部変数
    int _lpfValue; // ノイズ除去用

    // 内部関数
    int readSingleKnock();      // 単発ノック読み取り
    int getFilteredSensorValue(); // フィルタ済み値取得

public:
    DynamicAuth(int pinKnock, int pinRotary);
    void begin();

    // [処理3] ノックパターンの登録
    bool registerKnockSequence(LiquidCrystal &lcd);

    // [処理4] 回転位置の登録
    void registerRotationKey(LiquidCrystal &lcd);

    // [処理5] 認証実行
    bool authenticate(LiquidCrystal &lcd);
    
    void debugPrintKeys();
};

#endif