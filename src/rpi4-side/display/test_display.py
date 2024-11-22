from RPLCD.i2c import CharLCD
from time import sleep
import socket

# Initialize the LCD
lcd = CharLCD('PCF8574', 0x27)

# Clear display
lcd.clear()

# Write to display
lcd.write_string("IP Address:")

# Fetch and display IP
try:
    ip = socket.gethostbyname(socket.gethostname())
    lcd.cursor_pos = (1, 0)
    lcd.write_string(ip)
except Exception as e:
    lcd.cursor_pos = (1, 0)
    lcd.write_string("No IP Found")

# Keep display on
sleep(10)
lcd.clear()
