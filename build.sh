#!/bin/bash
arduino-cli compile -b arduino:avr:nano ~/Documents/software/DCC-MM2-controller
scp DCC-MM2-controller.arduino.avr.nano.hex  pi@192.168.1.84:/home/pi/temp
rm -r *.hex *.elf
ssh pi@192.168.1.84 <<'ENDSSH'
/home/pi/temp/upload.sh
#commands to run on remote host
ENDSSH

sleep 5
exit

