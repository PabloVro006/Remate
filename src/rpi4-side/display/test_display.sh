#!/bin/bash

LOGFILE="/home/pi/display_ip_debug.log"
exec > "$LOGFILE" 2>&1

echo "Script started at $(date)"

# I2C Address and Device
I2C_ADDR=0x27
LCD_DEVICE="/dev/i2c-1"

# Function to send commands
lcd_command() {
    echo "Sending command: $1"
    i2cset -y 1 $I2C_ADDR $1
    sleep 0.1  # Add a small delay
}

lcd_write_string() {
    local line=$1
    local text=$2
    echo "Writing to line $line: $text"
    for (( i=0; i<${#text}; i++ )); do
        char="${text:$i:1}"
        ascii=$(printf '%d' "'$char")
        echo "Sending char '$char' (ASCII: $ascii)"
        i2cset -y 1 $I2C_ADDR $ascii
        sleep 0.1  # Add delay between characters
    done
}

get_ip_address() {
    ip=$(hostname -I | awk '{print $1}')
    echo "Detected IP: ${ip:-No IP Found}"
    echo "${ip:-No IP Found}"
}

# Clear screen
lcd_command 0x01
echo "Screen cleared"

# Display IP
lcd_write_string 0 "IP Address:"
ip=$(get_ip_address)
lcd_write_string 1 "$ip"

echo "Script completed at $(date)"
