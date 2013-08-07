#!/bin/sh

cp -R ../bin/libaskue /usr/local/include/libaskue
cp ../bin/libaskue.h /usr/local/include/libaskue.h
cp ../bin/libaskue.so.2.0.0 /usr/local/lib/libaskue.so.2.0.0
ln -s /usr/local/lib/libaskue.so.2.0.0 /usr/local/lib/libaskue.so.2.0 
ln -s /usr/local/lib/libaskue.so.2.0.0 /usr/local/lib/libaskue.so.2
ln -s /usr/local/lib/libaskue.so.2.0.0 /usr/local/lib/libaskue.so 
