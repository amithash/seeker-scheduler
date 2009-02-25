#!/usr/bin/perl -w
use strict;
use Test::More (tests => 2);

BEGIN {use Chart::Gnuplot;}

my $temp = "temp.ps";

# Test default setting of chart title
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        title  => "My chart title",
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}

# Test formatting the chart title
{
    my $c = Chart::Gnuplot->new(
        output => $temp,
        title  => {
            text     => "My chart title in {/Symbol-Oblique greek}",
            font     => "Courier, 30",
            color    => "pink",
            offset   => "3,2",
            enhanced => "on",
        },
    );
    ok(ref($c) eq 'Chart::Gnuplot');
}
