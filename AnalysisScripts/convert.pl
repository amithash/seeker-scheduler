#!/usr/bin/perl

use strict;
use warnings;
if($#ARGV != 1){
	print "Usage: $0 IN_FILE OUT_DIR\n";
	exit;
}
my @bench_list = get_file('benchlist','#');
my $in_file = $ARGV[0];
my $out_dir = $ARGV[1];
system("mkdir $out_dir") unless(-d $out_dir);
system("mkdir $out_dir/sch") unless(-d "$out_dir/sch");

foreach my $bench (@bench_list){
	open(IN,"grep \"$bench\" $in_file |");
	my $line = <IN>;
	close(IN);
	chomp($line);
	my $pid;
	my $name;
	if($line =~ /p,(\d+),(.+)/){
		$pid = $1;
		$name = $2;
		print "Processing $name\n";
		open TMP,"grep -P \"s,\\d+,$pid,\" $in_file |" or die "could not grep on $in_file for $pid\n";
		open OUT,"+>$out_dir/sch/$name.tsv" or die "Could not create $out_dir/sch/$name.tsv\n";
		my @tmp = split(/\n/,join("",<TMP>));
		close(TMP);
		my $ipc;
		my $inst = 0.0;
		my $interval;
		my $req_st;
		my $giv_st;
		my $cpu;
		my $state;
		my $cy;
		foreach my $row (@tmp){
			my @line = split(/,/,$row);
			$interval = $line[1];
			$inst = $inst + ($line[4] * 1.0);
			$ipc = $line[5];
			$cpu = $line[3];
			$req_st = $line[6];
			$giv_st = $line[7];
			$state = $line[8];
			$cy = $line[9];
			print OUT "$inst $interval $cpu $ipc $req_st $giv_st $state $cy\n";
		}
		close(OUT);
	} else {
		print "Did not find a record of $bench\n";
	}
}
open TMP,"grep \"m,\" $in_file |" or die "Could not grep on $in_file for 'm,'\n";
my @tmp = split(/\n/,join("",<TMP>));
close(TMP);

open MUT_RE, "+>$out_dir/MUTATE_REQUESTED" or die "Could not open MUTATE_REQUESTED to write\n";
open MUT_GV, "+>$out_dir/MUTATE_GIVEN" or die "Could not open MUTATE_GIVEN to write\n";

my $print_once = 1;
foreach my $row (@tmp){
	if($row =~ /^m,(\d+),r,(.+),g,(.+)$/){
		my $interval = $1;
		my $req = join(" ",split(/,/,$2));
		my $giv = join(" ",split(/,/,$3));
		print MUT_RE "$interval $req\n";
		print MUT_GV "$interval $giv\n";
	} else {
		print "Not a good file\n" if($print_once == 1);
		$print_once = 0 if($print_once == 1);
	}
}
close(MUT_RE);
close(MUT_GV);


sub get_file{
	my $filename = shift;
	my $comment = shift;
	my @ret = ();
	open GF,$filename or die "Could not open $filename\n";
	while(my $line = <GF>){
		if($line !~ /^$comment/){
			chomp($line);
			$line =~ s/^\s+//;
			$line =~ s/\s+$//;
			push @ret,$line;
		}
	}
	close(GF);
	return @ret;
}


