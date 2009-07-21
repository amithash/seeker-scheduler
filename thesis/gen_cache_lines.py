#!/usr/bin/python

from pyx import *
import os, sys, re, string, operator, getopt, types

# Global Variables
labels = []


# Datafile should be the only argument
outputfile = sys.argv[1]
datafile = sys.argv[2]

# find the header line
infile = open(datafile,'r')
line = infile.readline()
commentline = None

# Find the last comment line before an uncommented line
# This should have the column headers
while (len(line) != 0):
	line = line.strip()

	if (line[0] in ["#","!","%"]):
		commentline = line
	else:
		break

	line = infile.readline()
 
# This should find some info for graphing
while (len(line) != 0):
	line = line.strip()

        if(line[0].isdigit):
                linearray = line.split(" ")
	line = infile.readline()

labels = commentline[1:].split(",")[1:]

infile.close()


# Do the line graph
	
g = graph.graphxy(width=9, height=4.5,
		  key=graph.key.key(pos="tl", hinside=0.9, dist=.1, vdist=0.3),
  		  y=graph.axis.linear(min=0, max=int(linearray[1]), title="Cache Conflicts"),
  		  y2=graph.axis.linear(min=0, max=int(linearray[2]), title="Time"),
		  x=graph.axis.linear(min=0, max=int(linearray[0]), title="Operation Count"))

# Actually draw the graph
g.plot([graph.data.file(datafile, x=1, y=2, title=labels[0])],
       [graph.style.line([color.grey.black, style.linestyle.solid])])
g.plot([graph.data.file(datafile, x=1, y2=3, context=locals(), title=labels[1])],
       [graph.style.line([color.grey.black, style.linestyle.dashed])])

g.plot([graph.data.file(datafile, x=1, y=4, title=labels[2])],
       [graph.style.line([color.rgb.red, style.linestyle.solid])])
g.plot([graph.data.file(datafile, x=1, y2=5, context=locals(), title=labels[3])],
       [graph.style.line([color.rgb.red, style.linestyle.dashed])])

g.plot([graph.data.file(datafile, x=1, y=6, title=labels[4])],
       [graph.style.line([color.rgb.green, style.linestyle.solid])])
g.plot([graph.data.file(datafile, x=1, y2=7, context=locals(), title=labels[5])],
       [graph.style.line([color.rgb.green, style.linestyle.dashed])])

#g.plot([graph.data.file(datafile, x=1, y=2, title=labels[0])],
#       [graph.style.line([color.grey.black, style.linestyle.solid])])
#g.plot([graph.data.file(datafile, x=1, y2=3, context=locals(), title=labels[1])],
#       [graph.style.line([color.grey.black, style.linestyle.dashed])])

#g.plot([graph.data.file(datafile, x=1, y=4, title=labels[2])],
#       [graph.style.line([color.grey(.35), style.linestyle.solid])])
#g.plot([graph.data.file(datafile, x=1, y2=5, context=locals(), title=labels[3])],
#       [graph.style.line([color.grey(.35), style.linestyle.dashed])])

#g.plot([graph.data.file(datafile, x=1, y=6, title=labels[4])],
#       [graph.style.line([color.grey(.55), style.linestyle.solid])])
#g.plot([graph.data.file(datafile, x=1, y2=7, context=locals(), title=labels[5])],
#       [graph.style.line([color.grey(.55), style.linestyle.dashed])])

g.writeEPSfile(outputfile)
