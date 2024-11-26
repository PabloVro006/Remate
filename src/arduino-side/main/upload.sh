#!/bin/bash
arduino-cli compile --fqbn arduino:megaavr:nona4809 main.ino
arduino-cli upload --fqbn arduino:megaavr:nona4809 -p /dev/cu.usbmodem1201 --verbose
