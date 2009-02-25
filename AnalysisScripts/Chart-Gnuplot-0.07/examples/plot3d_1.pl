#!/usr/bin/perl -w
use strict;
use Chart::Gnuplot;

# Data in Perl arrays
my @x = (0 .. 20);
my @y = (-10 .. 10);
my @z = map {rand()} @x;

my $chart = Chart::Gnuplot->new(
    output => "gallery/plot3d_1.png",
    title  => "3D plot from arrays of coordinates",
);

my $dataSet = Chart::Gnuplot::DataSet->new(
    xdata => \@x,
    ydata => \@y,
    zdata => \@z,
    style => 'lines',
);

$chart->plot3d($dataSet);
