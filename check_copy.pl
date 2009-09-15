#!/usr/bin/perl

use strict;
use warnings;
use Cwd;

my $regex = "Copyright\\s+2009";

my $this_dir = cwd();

my %allowed_exts = (
  "c" => 1,
  "h" => 1,
  "cpp" => 1,
  "C" => 1,
  "H" => 1,
  "pl" => 1,
  "py" => 1,
  "m" => 1,
  "Makefile" => 1,
  "README" => 1,
  "r" => 1,
  "sh" => 1
);

check($this_dir);

sub check
{
  my $dir = shift;
  if(-f $dir){
    check_file($dir);
  } else {
    my @list = <$dir/*>;
    foreach my $f (@list){
      if(-d $f){
        check($f);
      } elsif(-e $f){
        check_file($f);
      } else {
        print "ERR Do not know what to do to $f\n";
      }
    }
  }
}

sub check_file
{
  my $file = shift;

  my $ext = get_ext($file);
  if(not defined($allowed_exts{$ext})){
    return;
  }

  open IN,"$file" or die "Could not open $file\n";
  while(my $line = <IN>){
    if($line =~ /$regex/){
      close(IN);
      return;
    }
  }
  print "FAILED $file\n";
}

sub get_ext
{
  my $file = shift;
  if($file =~ /\//){
    my @tmp = split /\//,$file;
    $file = $tmp[$#tmp];
  }
  my @parts = split /\./,$file;
  my $ext = $parts[$#parts];

  return $ext;

}
