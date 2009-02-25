#!/usr/bin/perl -w
use strict;
use Chart::Gnuplot;

# Input data source
my @points = (
    [1, 1, 1.5],
    [1, 2, 1.8],
    [1, 3, 1.5],
    [2, 1, 1.8],
    [2, 2, 2],
    [2, 3, 1.8],
    [3, 1, 1.5],
    [3, 2, 1.8],
    [3, 3, 1.5],
);

my $chart = Chart::Gnuplot->new(
    output => "gallery/plot3d_2.png",
    title  => "3D plot from Perl array of points",
);


my $dataSet = Chart::Gnuplot::DataSet->new(
    points => \@points,
);

$chart->plot3d($dataSet);
