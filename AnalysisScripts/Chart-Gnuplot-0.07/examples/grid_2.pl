#!/usr/bin/perl -w
use strict;
use Chart::Gnuplot;

# - Minor grid lines will be added if the minor axis tics are added
# - Minor grid lines cannot be drawn if the minor axis tics are not added

my $chart = Chart::Gnuplot->new(
    output => "gallery/grid_2.png",
    title  => "Add minor grid lines",
    xlabel => "major and minor grid lines",
    ylabel => "only minor grid lines",

    xtics  => {
        minor => 4,
    },
    ytics  => {
        minor => 2,
    },
    grid   => {
        xlines => 'on, on',     # draw major and minor grid lines
        ylines => 'off, on',    # no major grid lines
    },
);

my $dataSet = Chart::Gnuplot::DataSet->new(
    func => "sin(x)",
);

$chart->plot2d($dataSet);
