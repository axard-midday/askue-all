#!/bin/sh

export LIBRARY_PATH=/usr/lib/i386-linux-gnu:$LIBRARY_PATH

gcc-3.4 -Wall main.c -o askue-port -std=c99 -laskue -lreadline -D_AP_TBUFFER_LENGTH=1024 -D_AP_HISTORY_LENGTH=10

strip -s askue-port
