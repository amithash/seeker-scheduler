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

labels = commentline[1:].split()[1:]

infile.close()


# Do the line graph
	
g = graph.graphxy(width=9, height=4.5,
		  key=graph.key.key(pos="br", hinside=1, dist=.1, vdist=1.3),
  		  y=graph.axis.linear(min=0, max=70, title="Compression Ratio"),
		  x=graph.axis.linear(min=0, max=75000000, title="Operation Count"))

# Actually draw the graph

g.plot([graph.data.file(datafile, x=1, y=2, title=labels[0])],
       [graph.style.line([color.grey.black, style.linestyle.solid])])
g.plot([graph.data.file(datafile, x=1, y=3, title=labels[1])],
       [graph.style.line([color.grey.black, style.linestyle.dashed])])
g.plot([graph.data.file(datafile, x=1, y=4, title=labels[2])],
       [graph.style.line([color.grey(.35), style.linestyle.solid])])
g.plot([graph.data.file(datafile, x=1, y=5, title=labels[3])],
       [graph.style.line([color.grey(.35), style.linestyle.dashed])]) 
g.writeEPSfile(outputfile)
