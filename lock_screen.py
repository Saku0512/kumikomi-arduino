import tkinter as tk

root = tk.Tk()
root.attributes("-fullscreen", True)
root.configure(bg="black")

# Alt+F4無効化
root.protocol("WM_DELETE_WINDOW", lambda: None)

# ESCで解除（デバッグ用）
def unlock(event):
    root.destroy()

root.bind("<Escape>", unlock)

label = tk.Label(
    root,
    text="LOCKED",
    fg="white",
    bg="black",
    font=("Arial", 48)
)
label.pack(expand=True)

root.mainloop()
