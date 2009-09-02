#!/usr/bin/perl

use strict;
use warnings;

foreach my $f (@ARGV){
	if(-e $f){
		print_for_file($f);
	}
	if(-d $f){
		print_for_dir($f);
	}
}

sub print_for_dir
{
	my $d = shift;
	my @cont = <$d/*>;
	print "=======$d=======\n";
	foreach my $f (@cont){
		if(-e $f){
			print_for_file($f);
		}
		if(-d $f){
			print_for_dir($f);
		}
	}
}

sub print_for_file
{
	my $inf = shift;
	open IN, "$inf" or die "Could not open $inf\n";
	print "-------$inf-------\n";
	while(my $line = <IN>){
		chomp($line);
		my $func_name = is_func($line);
		if($inf =~ /\.c$/){
			if($func_name ne "NULL"){
				print "$func_name\n";
				next;
			}
		}
		$func_name = is_macro($line);
		if($func_name ne "NULL"){
			print "MACRO: $func_name\n";
			next;
		}
	}
	close(IN);
}


sub is_func
{
	my $l = shift;
	my @ret_types = (
		"void",
		"int",
		"unsigned int",
		"unsigned long",
		"unsigned long int",
		"unsigned long long",
		"unsigned long long int",
		"char",
		"unsigned char",
		"long",
		"long long",
		"long int",
		"long long int"
	);
	my @discreptors = (
		"",
		"static",
		"inline"
	);
	my $func;

	foreach my $key (@ret_types){
		foreach my $dis (@discreptors){
			if($l =~ /^\s*$dis\s*$key\s+(\S+)\s*\(/){
				return $1;
			}
		}
	}
	return "NULL";
}

sub is_macro
{
	my $l = shift;
	my $macro;
	if($l =~/^\s*#define\s+([\d\S]+)\(/){
		return $1;
	}
	return "NULL";
}


