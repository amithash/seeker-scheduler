#!/usr/bin/perl -w
use strict;
use Chart::Gnuplot;

my $chart = Chart::Gnuplot->new(
    output => "gallery/grid_6.png",
    xtics  => {
        minor => 3,
    },
    grid   => {
        linetype => "longdash, dot-longdash",
        xlines   => "on, on"
    },
);

my $dataSet = Chart::Gnuplot::DataSet->new(
    func => "sin(x)",
);

$chart->plot2d($dataSet);
