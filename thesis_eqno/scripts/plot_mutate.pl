#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long;

my $pl = "/usr/bin/ploticus";
$pl = "./pl" if(`which $pl` !~ /^\//);
print "Using: $pl\n";

my $area_string = "
#proc getdata
file: MUT_GIV
delim: space

filter:
##set X = \$ref(1)
##set R5 = \$ref(6)
##set R4 = \$ref(5)
##set R3 = \$ref(4)
##set R2 = \$ref(3)
##set R1 = \$ref(2)
##set OUT5 = \$arithl(\@R1+\@R2+\@R3+\@R4+\@R5)
##set OUT4 = \$arithl(\@R1+\@R2+\@R3+\@R4)
##set OUT3 = \$arithl(\@R1+\@R2+\@R3)
##set OUT2 = \$arithl(\@R1+\@R2)
##print \@X	\@R1	\@OUT2	\@OUT3	 \@OUT4    \@OUT5

#endproc

#proc areadef
  rectangle: 1 1 6.5 6.5
  xautorange: datafields=1 combomode=normal incmult=2.0 nearest=auto
  yrange: 0 5
//  yautorange: datafields=2,3,4,5,6 combomode=normal incmult=2.0  nearest=auto

#proc xaxis
  stubs: inc 
  label: Mutation Interval

#proc yaxis
  stubs: inc 
  stubcull: yes

#proc lineplot 
 xfield: 1
 yfield: 6
 fill: rgb(1,0,0)
 legendlabel: P4

#proc lineplot 
 xfield: 1
 yfield: 5
 fill: rgb(0.33,0,0)
 legendlabel: P3

#proc lineplot 
 xfield: 1
 yfield: 4
 fill: rgb(0.33,0.33,0)
 legendlabel: P2

#proc lineplot
 xfield: 1
 yfield: 3
 fill: rgb(0,0.33,0)
 legendlabel: P1

#proc lineplot
 xfield: 1
 yfield: 2
 fill: rgb(0,1,0)
 legendlabel: P0

#proc legend
 location: min+1 min+5.0
 textdetails: size=8
 format: singleline
 sep: 1.0
";
my $area_string2 = "
#proc getdata
file: MUT_GIV
delim: space

filter:
##set X = \$ref(1)
##set R2 = \$ref(3)
##set R1 = \$ref(2)
##set OUT2 = \$arithl(\@R1+\@R2)
##print \@X	\@R1	\@OUT2

#endproc

#proc areadef
  rectangle: 1 1 6.5 6.5
  xautorange: datafields=1 combomode=normal incmult=2.0 nearest=auto
  yrange: 0 5
//  yautorange: datafields=2,3 combomode=normal incmult=2.0  nearest=auto

#proc xaxis
  stubs: inc 
  label: Mutation Interval

#proc yaxis
  stubs: inc 
  stubcull: yes

#proc lineplot 
 xfield: 1
 yfield: 3
 fill: rgb(1,0,0)
 legendlabel: P1

#proc lineplot 
 xfield: 1
 yfield: 2
 fill: rgb(0,1,0)
 legendlabel: P0

#proc legend
 location: min+1 min+5.0
 textdetails: size=8
 format: singleline
 sep: 1.0
";

my $pow_string = "
#proc getdata
file: IN_DATA
delim: space
#endproc

#proc areadef
  rectangle: 1 1 6.5 6.5
  xautorange: datafields=1 combomode=normal incmult=2.0 nearest=auto
  yautorange: datafields=2 combomode=normal incmult=2.0  nearest=auto

#proc xaxis
  stubs: inc 
  label: Mutation Interval

#proc yaxis
  stubs: inc 
  stubcull: yes
  label: Power (Watts)

#proc lineplot 
 xfield: 1
 yfield: 2
 linedetails: width=1 color=rgb(1,0,0)
";

sub help
{
	my $error = shift;
	if(defined($error)){
		print "$error\n";
	}
	print "Usage: $0 -in <Input Log Dir> -out <Output dir for plots> [-type = <image type>, default: png]\n";
	exit;
}

my $input; 
my $output;
my $type;
my $help;
my $giv_c = 0;
my $req_c = 0;
GetOptions(
	'in=s' => \$input,
	'out=s' => \$output,
	'type=s' => \$type,
	'help' => \$help
);
help() if(defined($help));
help("Error input and output param are mandatory") if(not defined($input) or not defined($output));
$type = "png" if(not defined($type));

unless(-d $output){
	system("mkdir -p $output");
}
help("$input does not exist!") unless(-d $input);

my $giv = "$input/MUT_GIV";
my $req = "$input/MUT_REQ";
help("Error: The output does not seem to contain MUT_GIV or MUT_GIV.gz") if(! -e $giv and ! -e "$giv.gz");
help("Error: The output does not seem to contain MUT_REQ or MUT_REQ.gz") if(! -e $req and ! -e "$req.gz");

print "Starting\n";

# Plot the power data. and figure out the total number of states.

open IN, "cat $giv.gz | gunzip |" or die "Could not open $giv.gz to read\n";
open OUT, "+>$output/tmp.dat" or die "Could not create $output/tmp.dat";
my $states;

my @pow5 = (64.6,75.6,88.8,108.2,115.0);
my @pow2 = (64.6,115.0);

my @pow;

while(my $line = <IN>){
	chomp($line);
	my @c = split(/\s/,$line);
	if(not defined($states)){
		$states = $#c;
		unless($states == 5 or $states == 2){
			print "Warning: Garbage in, garbage out. $states is neither 2 or 5, this script cannot handle this file = $req";
		}
		if($states == 2){
			@pow = @pow2;
		} else {
			@pow = @pow5
		}
	}
	my $this_power = 0;
	for(my $i=0;$i<$states;$i++){
		$this_power += $pow[$i] * $c[$i+1]; # power per proc times no proc
	}
	print OUT "$c[0] $this_power\n";
}
close(IN);
close(OUT);

my $pow_p = $pow_string;
my $tmp = "$output/tmp.dat";
$pow_p =~ s/IN_DATA/$tmp/;
open TMP, "+>$output/tmp.plot" or die "Could not create $output/tmp.dat";
print TMP "$pow_p";
system("$pl $output/tmp.plot -maxrows 100000000 -o $output/pow.$type -$type");
system("rm $output/tmp.plot");
system("rm $output/tmp.dat");

# Plot the req and giv.
my $req_s;
my $giv_s;
if($states == 5){
	$req_s = $area_string;
	$giv_s = $area_string;
} else {
	$req_s = $area_string2;
	$giv_s = $area_string2;
}
system("cat $req.gz | gunzip > $output/tmp_req.dat");
system("cat $giv.gz | gunzip > $output/tmp_giv.dat");
$req_s =~ s/MUT_GIV/$output\/tmp_req.dat/;
$giv_s =~ s/MUT_GIV/$output\/tmp_giv.dat/;

open TMP, "+>$output/tmp.plot" or die "Could not create $output/tmp.plot";
print TMP "$req_s";
close(TMP);
system("$pl $output/tmp.plot -maxrows 100000000 -o $output/req.$type -$type");
system("rm $output/tmp.plot");

open TMP, "+>$output/tmp.plot" or die "Could not create $output/tmp.plot";
print TMP "$giv_s";
close(TMP);
system("$pl $output/tmp.plot -maxrows 100000000 -o $output/giv.$type -$type");
system("rm $output/tmp.plot");

#system("rm $output/tmp_req.dat");
#system("rm $output/tmp_giv.dat");
# End compress things which were uncompressed.
