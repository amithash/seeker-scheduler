#!/bin/sh

#make clean
#make
#make thesis.bbl
#make new
#make
#make new
#make
#make thesis.dvi

make clean 
make 
rm thesis.pdf
make > /dev/null

