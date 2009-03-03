#!/usr/bin/perl

use strict;
use warnings;
use lib "$ENV{SEEKER_HOME}/lib/Chart-Gnuplot-0.07/lib";
use Chart::Gnuplot;



my @x = (-10 .. 10);
my @y = (0 .. 20);
plotxy(\@x,\@y,"simple.png","Omg this is simple");

sub plotxy{
	my $x_ref = shift;
	my $y_ref = shift;
	my $file = shift;
	my $title = shift || "";
	my $xlabel = shift || "";
	my $ylabel = shift || "";
        
	# Create chart object and specify the properties of the chart
	my $chart = Chart::Gnuplot->new(
	    output => $file,
	    title  => { text => $title, font => "DejaVuSans-Bold.ttf, 12, bold"},
	    xlabel => $xlabel,
	    ylabel => $ylabel,
	    bg     => "white"
	#   ....
	);
	                  
	# Create dataset object and specify the properties of the dataset
	my $dataSet = Chart::Gnuplot::DataSet->new(
	xdata => $x_ref,
	ydata => $y_ref,
	# Avaliable: [xdata, ydata, zdata (for 3d plots)] => reference to array of x,y,z values
	#            [points => [[x1,y1,z1],[x1,y1,z1],[x1,y1,z1],...]
	#	     file => "filename"

	title => "legend for this",

	style => "boxes" #,
	# Avaliable: 'lines', 'points', 'linespoints', 'dots', 'impulses',
	#            'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps',
	#            'histeps', 'filledcurves', 'boxes', 'boxerrorbars', 'boxxyerrorbars',
	#            'vectors', 'financebars', 'candlesticks', 'errorlines', 'xerrorlines',
	#            'yerrorlines', 'xyerrorlines', 'pm3d', 'labels', 'histograms',
	#            'image', 'rgbimage'

	);
                                                    
	# Plot the data set on the chart
	$chart->plot2d($dataSet);

	# for 3d plots,
	#$chart->plot3d($dataSet);

	# plot multiple lines on the same graph. 
	#$chart->plot2d($dataSet1,$dataSet2);
}

