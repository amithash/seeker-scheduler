#!/usr/bin/python
import sys


if len(sys.argv) < 3:	
        print "I need more cowbell"
else:	
        inputfile = open(sys.argv[1], 'r')
        outputfile = open(sys.argv[2], 'w')

        for line in inputfile:
                if(line[0] != "#"):
                        line = line.replace(","," ")
                outputfile.write(line)
        

        inputfile.close()
        outputfile.close()

                                                
