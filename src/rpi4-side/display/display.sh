#!/bin/bash

# Path to the I2C LCD command tool (install i2c-tools if not installed)
I2C_ADDR=0x27  # Adjust this to your LCD's I2C address
LCD_DEVICE="/dev/i2c-1"

# Function to send commands to the LCD
lcd_command() {
    /usr/sbin/i2cset -y 1 $I2C_ADDR $1
}

lcd_write_string() {
    local line=$1
    local text=$2
    for (( i=0; i<${#text}; i++ )); do
        char="${text:$i:1}"
        # Convert character to ASCII and send to LCD
        ascii=$(printf '%d' "'$char")
        i2cset -y 1 $I2C_ADDR $ascii
    done
}

get_ip_address() {
    # Try to get the IP address of wlan0, eth0, or fallback to "No IP"
    ip=$(hostname -I | awk '{print $1}')
    echo "${ip:-No IP Found}"
}

# Main script logic
lcd_command 0x01  # Clear the screen
lcd_write_string 0 "IP Address:"
ip=$(get_ip_address)
lcd_write_string 1 "$ip"  # Print the IP on the second line
