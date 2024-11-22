from RPLCD.i2c import CharLCD
from time import sleep
import subprocess

# Initialize the LCD
lcd = CharLCD('PCF8574', 0x27)

# Clear display
lcd.clear()

# Write to display
lcd.cursor_pos = (0, 0)
lcd.write_string("IP Address:")

# Fetch and display IP
try:
  ip = subprocess.check_output("hostname -I", shell=True).decode("utf-8").strip().split()[0]
  #ip = socket.gethostbyname(socket.gethostname())
  lcd.cursor_pos = (1, 0)
  lcd.write_string(ip)
except Exception as e:
  lcd.cursor_pos = (1, 0)
  lcd.write_string("No IP Found")
