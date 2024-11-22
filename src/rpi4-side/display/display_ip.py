from RPLCD.i2c import CharLCD
from time import sleep
import subprocess

# Initialize the LCD
lcd = CharLCD('PCF8574', 0x27)

# Function to clear a specific row
def clear_display(row):
  lcd.cursor_pos = (row, 0)
  lcd.write_string(" " * 16)
  lcd.cursor_pos = (row, 0)

# Function to get the current Wi-Fi SSID
def get_wifi_name():
    try:
        ssid = subprocess.check_output("iwgetid -r", shell=True).decode("utf-8").strip()
        return ssid if ssid else "No Wi-Fi"
    except Exception as e:
        return "No Wi-Fi"

# Function to get the current IP address
def get_ip_address():
    try:
        ip = subprocess.check_output("hostname -I", shell=True).decode("utf-8").strip().split()[0]
        return ip if ip else "No IP Found"
    except Exception as e:
        return "No IP Found"

# Clear display
lcd.clear()

# Fetch and display IP
try:
  while True:
    # Fetch Wi-Fi name and IP address
    wifi_name = get_wifi_name()
    ip = get_ip_address()
    clear_display(0)
    lcd.write_string(wifi_name[:16])  # Truncate if longer than 16 characters
    clear_display(1)
    lcd.write_string(ip)
    sleep(60)

except KeyboardInterrupt:
  lcd.clear()
