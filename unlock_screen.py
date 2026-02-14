import tkinter as tk

root = tk.Tk()
root.attributes("-fullscreen", True)
root.configure(bg="black")

# Alt+F4無効化
root.protocol("WM_DELETE_WINDOW", lambda: None)

label = tk.Label(
    root,
    text="UNLOCKED",
    fg="white",
    bg="black",
    font=("Arial", 48)
)
label.pack(expand=True)

root.mainloop()