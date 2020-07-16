#!/bin/bash

arduino-cli compile  -b arduino:avr:nano ~/Documents/software/DCC-MM2-controller
###-p "COM3"###
# arduino-cli compile  -b arduino:avr:nano ~/Documents/software/DCC-MM2-controller
#scp DCC-MM2-controller.arduino.avr.nano.hex pi@192.168.1.84:/home/pi/temp/
#ssh pi@192.168.1.84 <<'ENDSSH'
#commands to run on remote host
#/home/pi/temp/upload.sh
#ENDSSH

#arduino-cli upload -b arduino:avr:nano:cpu=atmega328old -p COM3 -i ~/Documents/software/DCC-MM2-controller/DCC-MM2-controller.arduino.avr.nano.hex LOCLA UPLOADING
#rm *.hex *.elf
exit
