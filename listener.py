import serial
import subprocess
import time

SERIAL_PORT = "/dev/ttyACM0"
BAUDRATE = 9600 # Arduino側の設定に合わせてください (9600 or 115200)
LOCK_WORD = "!!LOCK!!"
UNLOCK_WORD = "!!UNLOCK!!"

ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)

print("Listening serial...")

# ロック画面のプロセスを保持する変数
lock_process = None

try:
    while True:
        try:
            line = ser.readline().decode(errors="ignore").strip()
        except serial.SerialException:
            print("Serial disconnect/error. Retrying...")
            time.sleep(1)
            continue

        if not line:
            continue

        print("Recv:", line)

        # ロック命令受信
        if LOCK_WORD in line:
            # すでに起動していない場合のみ起動
            if lock_process is None or lock_process.poll() is not None:
                print("Lock command received. Launching lock screen...")
                # Popenの戻り値（プロセスオブジェクト）を変数に保存
                lock_process = subprocess.Popen(["python3", "lock_screen.py"])
            else:
                print("Lock screen is already running.")

        # 解除命令受信
        elif UNLOCK_WORD in line:
            if lock_process and lock_process.poll() is None:
                print("Unlock command received. Killing lock screen...")
                lock_process.terminate() # プロセスを終了させる
                lock_process.wait()      # 完全に終了するのを待つ
                lock_process = None      # 変数をリセット
            else:
                print("Lock screen is not running.")

except KeyboardInterrupt:
    print("\nExiting serial listener...")
    # serial.py終了時にロック画面が開いていたら道連れにして閉じる
    if lock_process and lock_process.poll() is None:
        lock_process.terminate()
    ser.close()