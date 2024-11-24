#!/bin/bash
folder=build-uno
[ -d "$folder" ] && rm -r "$folder"
make
/usr/share/arduino/hardware/tools/avr/bin/avrdude -q -V -p atmega328p -C /etc/avrdude.conf -D -c arduino -b 115200 -P /dev/ttyACM* -U flash:w:build-uno/main_arduino_.hex:i
