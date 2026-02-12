#ifndef DYNAMIC_AUTH_H
#define DYNAMIC_AUTH_H

#include <Arduino.h>
#include <LiquidCrystal.h>

// パスワードの桁数
#define PASSWORD_LENGTH 4

// 方向の定義
enum Direction {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_CENTER
};

class DynamicAuth {
private:
    int _pinX;
    int _pinY;
    int _pinZ;
    
    // 中心値（キャリブレーション用）
    int _offsetX;
    int _offsetY;

    // 保存されたパスワード（正解データ）
    Direction _secretCode[PASSWORD_LENGTH];

    // 内部関数: 方向入力を1回読み取る（入力があるまでブロックする）
    Direction waitForInput(LiquidCrystal &lcd);
    
    // 方向を文字列に変換（表示用）
    String dirToString(Direction dir);

public:
    DynamicAuth(int pinX, int pinY, int pinZ);
    void begin();

    // ジョイスティックのキャリブレーション（起動時に呼ぶ）
    void calibrate();

    Direction getCurrentDirection();

    // パスワード設定（短押し）
    void setPassword(LiquidCrystal &lcd);

    // 認証実行（長押し）
    bool authenticate(LiquidCrystal &lcd);
};

#endif