# システム構成図

このドキュメントでは、組み込みArduino認証システムのアーキテクチャについて説明します。

## 1. 全体構成図 (System Overview)

Raspberry Pi Picoを中心としたハードウェアデバイスと、PC上のPythonスクリプトが連携して動作します。

```mermaid
graph LR
    subgraph Device ["認証デバイス (Device)"]
        Pico[Raspberry Pi Pico]
        
        subgraph Input ["入力デバイス"]
            Joy[アナログジョイスティック]
            Btn["スイッチ (Z軸)"]
        end
        
        subgraph Output ["出力デバイス"]
            LCD["LCDディスプレイ (16x2)"]
            LED[状態表示LED]
        end
        
        Joy -->|"X/Y軸 (Analog)"| Pico
        Btn -->|"Click (Digital)"| Pico
        Pico -->|"I2C/GPIO"| LCD
        Pico -->|"GPIO"| LED
    end

    subgraph Host ["ホストPC (Host PC)"]
        Listener["listener.py<br>(Serial監視)"]
        LockApp["lock_screen.py<br>(画面ロック)"]
        
        Listener -.->|"Subprocess起動/終了"| LockApp
    end

    Pico ==>|"USB Serial (CDC)<br>Command: !!LOCK!! / !!UNLOCK!!"| Listener

    style Device fill:#f9f,stroke:#333,stroke-width:2px
    style Host fill:#ccf,stroke:#333,stroke-width:2px
```

## 2. ソフトウェア構成 (Software Structure)

### ファームウェア (Raspberry Pi Pico)
- **Framework**: Arduino
- **Files**:
  - `src/main.cpp`: メイン制御ロジック。UI状態管理、周辺機器の初期化。
  - `lib/DynamicAuth/`: 認証ロジックライブラリ。
    - `DynamicAuth.h/cpp`: パスワード管理、入力パターン判定、キャリブレーション機能を提供。

### ホストアプリケーション (Python)
- **Files**:
  - `listener.py`: シリアルポート(`COM`/`/dev/tty*`)を監視し、コマンドに応じてロック画面を制御します。
  - `lock_screen.py`: `tkinter`を使用したフルスクリーンのロック画面オーバーレイ。

## 3. 処理シーケンス (Process Flow)

ユーザーが認証操作を行った際のデータの流れです。

```mermaid
sequenceDiagram
    participant User as ユーザー
    participant Pico as Pico (Firmware)
    participant Listener as PC (listener.py)
    participant LockScreen as PC (lock_screen.py)

    Note over User, Pico: 認証プロセス開始

    User->>Pico: ジョイスティック上を3秒長押し
    Pico->>Listener: Serial "!!LOCK!!" 送信
    Listener->>LockScreen: サブプロセス起動 (python lock_screen.py)
    LockScreen-->>User: 画面暗転・ロック表示

    loop パスワード入力 (4回)
        Pico->>User: LCD "Enter Code #N"
        User->>Pico: 方向入力
        Pico-->>Pico: パターン照合
    end

    alt 認証成功
        Pico->>Listener: Serial "!!UNLOCK!!" 送信
        Listener->>LockScreen: プロセス終了 (Terminate)
        LockScreen-->>User: ロック解除
        Pico->>User: LCD "!!UNLOCK!!" / LED点灯
    else 認証失敗
        Pico->>User: LCD "Auth Failed"
        Note right of Listener: ロック画面は維持される
    end
```

## 4. 通信プロトコル

PicoとPC間はUSBシリアル通信（ボーレート設定に依存、listener.pyでは9600または115200を使用）で行われます。

| 送信元 | コマンド | 説明 |
|:---|:---|:---|
| Pico | `!!LOCK!!` | パソコンの画面をロックする（ロック画面アプリを起動） |
| Pico | `!!UNLOCK!!` | パソコンの画面をロック解除する（ロック画面アプリを終了） |
