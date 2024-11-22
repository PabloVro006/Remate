from RPLCD.i2c import CharLCD
from time import sleep
import subprocess

# Initialize the LCD
lcd = CharLCD('PCF8574', 0x27)

def clear_display(row):
  lcd.cursor_pos = (row, 0)
  lcd.write_string(" " * 16)
  lcd.cursor_pos = (row, 0)

def get_ip_address():
  try:
    ip = subprocess.check_output("hostname -I", shell=True).decode("utf-8").strip.split()[0]
    if(ip):
      return ip
    else:
      return "No IP Found"
  except Exception as e:
    return "No IP Found"

# Clear display
lcd.clear()

# Write to display
lcd.cursor_pos = (0, 0)
lcd.write_string("IP Address:")

# Fetch and display IP
try:
  while True:
    ip = get_ip_address()
    clear_display(1)
    lcd.write_string(ip)
    sleep(60)

except KeyboardInterrupt:
  lcd.clear()
