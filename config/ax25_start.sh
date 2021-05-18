#!/bin/sh

# You have to have something like this in /etc/ax25/axports
# radio HA5OGL-15 0 255 2 300 bps HF

#These commands shell run as root.

kissattach /dev/pts/0 radio
kissparms -c 1 -p radio
