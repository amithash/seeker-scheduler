#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $pm = "/thesis/ms_thesis-amithash_prasad/scripts/plot_mutate.pl";
my $tmp = "/thesis/tmp/";
my $t = "jpeg";
my $this = cwd();
#chdir("/thesis/ms_thsis-amithash_prasad/scripts");
my @dirs = <$this/log_*>;
my $out = "/thesis/mut_sel";
system("mkdir $out") unless(-d $out);

foreach my $dir (@dirs){
	if($dir =~ /log_(\d+)_(\d+)_([a-zA-Z-]+)$/){
		my $interval = $1;
		my $delta = $2;
		my $wrk = $3;
		print "$interval : $delta : $wrk\n";
		system("$pm -in $dir -out $tmp -type $t");
		system("mkdir $out/$wrk") unless(-d "$out/$wrk");
		system("mv $tmp/giv.$t $out/$wrk/${interval}_${delta}.$t");
		system("rm $tmp/*");
	}
}
#chdir($this);
