#!/usr/bin/perl -w
use strict;
use Test::More (tests => 4);

BEGIN {use Chart::Gnuplot;}

my $temp = "temp.ps";

# Test plotting from Perl arrays of x coordinates and y coordinates
{
    my @x = (-10 .. 10);
    my @y = (0 .. 20);

    my $c = Chart::Gnuplot->new(
        output => $temp,
    );
    my $d = Chart::Gnuplot::DataSet->new(
        xdata => \@x,
        ydata => \@y,
    );
    ok(ref($c) eq 'Chart::Gnuplot' && ref($d) eq 'Chart::Gnuplot::DataSet');
}

# Test plotting from Perl array of x-y pairs
{
    my @xy = (
        [1, 2],
        [3, 1],
        [5, 0],
        [6, 1],
        [7, 2],
        [9, 3],
    );

    my $c = Chart::Gnuplot->new(
        output => $temp,
    );
    my $d = Chart::Gnuplot::DataSet->new(
        points => \@xy,
    );
    ok(ref($c) eq 'Chart::Gnuplot' && ref($d) eq 'Chart::Gnuplot::DataSet');
}

# Test plotting from data file
{
    my $infile = "t/data.dat";
    $infile = "data.dat" if (!-e "t/data.dat");

    my $c = Chart::Gnuplot->new(
        output => $temp,
    );
    my $d = Chart::Gnuplot::DataSet->new(
        datafile => $infile,
    );
    ok(ref($c) eq 'Chart::Gnuplot' && ref($d) eq 'Chart::Gnuplot::DataSet');
}

# Test plotting from mathematical expression
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
    );
    my $d = Chart::Gnuplot::DataSet->new(
        func => "sin(x)",
    );
    ok(ref($c) eq 'Chart::Gnuplot' && ref($d) eq 'Chart::Gnuplot::DataSet');
}
