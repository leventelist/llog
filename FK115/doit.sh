#!/bin/sh

rm -f eprom.*
rm -f channel_list.txt

./FK115_calc.pl > channel_list.txt
hexdump -C eprom.bin > eprom.txt

