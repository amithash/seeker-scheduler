#!/usr/bin/perl -w
use strict;
use Test::More (tests => 2);

BEGIN {use Chart::Gnuplot;}

my $temp = "temp.ps";

# Test default setting of axis labels
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        xlabel  => 'x-label',
        ylabel  => 'y-label',
        x2label => 'x2-label',
        y2label => 'y2-label',
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}

# Test formatting the axis labels
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        xlabel => {
            text     => "My axis label in {/Symbol-Oblique greek}",
            font     => "Courier, 30",
            color    => "pink",
            offset   => "3,2",
            enhanced => "on",
        },
        ylabel => {
            text   => "Rotated 80 deg",
            rotate => 80,
        },
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}
