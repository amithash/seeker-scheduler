#!/usr/bin/perl -w
use strict;
use Test::More (tests => 2);

BEGIN {use Chart::Gnuplot;}

my $temp = "temp.ps";

# Test default setting of gridlines
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        grid   => 'on',
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}

# Test formatting the gridlines
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        grid   => {
            xlines   => "on, on",
            ylines   => "on, off",
            linetyle => "longdash, dot-longdash",
            width    => "2,1",
        },
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}
