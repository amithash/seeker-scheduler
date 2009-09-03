#!/usr/bin/perl

use strict;
use warnings;

my %file_macros;
my %macro_files;
my %file_funcs;
my %func_files;
my %files_all;
my $file_count = 0;

my %func_uses;
my %macro_uses;

foreach my $f (@ARGV){
	if(-e $f){
		parse_def_file($f);
	}
	if(-d $f){
		parse_def_dir($f);
	}
}

open OUT,"+>figure.dot" or die "Could not create figure.dot!\n";

# XXX Print DOT header.
print OUT "digraph G {\n";
foreach my $file (keys %files_all){
  if((not defined($file_funcs{$file})) and (not defined($file_macros{$file}))){
    next;
  }
  print OUT "/* $file */\n";
  print OUT "subgraph cluster_$files_all{$file} {\n";
  print OUT "label=\"$file\"\n";
  print OUT "shape=\"rectangle\"\n";
  if(defined($file_macros{$file})){
    print OUT "subgraph cluster_$files_all{$file}_macro {\n";
    print OUT "label=\"MACRO\"\n";
    print OUT "shape=\"rectangle\"\n";
    my $ref = $file_macros{$file};
    foreach my $func (@$ref){
      print OUT "$func\n";
    }
    print OUT "}\n";
  }
  if(defined($file_funcs{$file})){
    print OUT "subgraph cluster_$files_all{$file}_funcs {\n";
    print OUT "label=\"FUNCTIONS\"\n";
    print OUT "shape=\"rectangle\"\n";
    my $ref = $file_funcs{$file};
    foreach my $func (@$ref){
      print OUT "$func\n";
    }
    print OUT "}\n";
  }
  print OUT "}\n\n";
}
print OUT "}\n";

# XXX Print the function structures based on files. 

# Everything is in %funcs and %macros. 
# XXX From here, as and when we find a use, we start adding an arrow. 
# Design descision, uses of functions not defined goes in a seperate DUMMY FILE called "ExternalDependancies"
# Make the ExternalDependancies structure "editable" so people can re-group them...
foreach my $f (@ARGV){
  if(-e $f){
    parse_uses_file($f);
  }
  if(-d $f){
    parse_uses_dir($f);
  }
}
close(OUT);

# XXX Once this is implemented, remove further code! 


print "++++++++++++++++++++++++++++++++FUNCS+++++++++++++++++++++++++++++++\n";
foreach my $f (keys %file_funcs){
  my $ref  = $file_funcs{$f};
  foreach my $func (@$ref){
    print "$f : $func\n";
  }
}
print "++++++++++++++++++++++++++++++++MACROS+++++++++++++++++++++++++++++++\n";
foreach my $f (keys %file_macros){
  my $ref  = $file_macros{$f};
  foreach my $func (@$ref){
    print "$f : $func\n";
  }
}

sub parse_def_dir
{
	my $d = shift;
	my @cont = <$d/*>;
	foreach my $f (@cont){
		if(-e $f){
			parse_def_file($f);
		}
		if(-d $f){
			parse_def_dir($f);
		}
	}
}

sub parse_uses_dir
{
	my $d = shift;
	my @cont = <$d/*>;
	foreach my $f (@cont){
		if(-e $f){
			parse_uses_file($f);
		}
		if(-d $f){
			parse_uses_dir($f);
		}
	}
}

sub parse_def_file
{
	my $inf = shift;

  if(-d $inf){
    return;
  }
  if($inf !~ /\.c$/ and $inf !~ /\.h$/){
    return;
  }
  $files_all{$inf} = $file_count;
  $file_count++;
	open IN, "$inf" or die "Could not open $inf\n";
	while(my $line = <IN>){
		chomp($line);
		my $func_name = is_func_def($line);
		if($inf =~ /\.c$/){
			if($func_name ne "NULL"){
        if(defined($file_funcs{$inf})){
          push @{$file_funcs{$inf}}, $func_name;
        } else {
          $file_funcs{$inf} = [$func_name];
        }
        if(defined($func_files{$func_name})){
          if(exists_in_array($inf,$func_files{$func_name})){
            print STDERR "WARNING: Re-def of function $func_name in the same file $inf assumed to be featurization\n";
          } else {
            push @{$func_files{$func_name}}, $inf;
          }
        } else {
          $func_files{$func_name} = [$inf];
        }
				next;
			}
		}
		$func_name = is_macro($line);
		if($func_name ne "NULL"){
        if(defined($file_macros{$inf})){
          push @{$file_macros{$inf}}, $func_name;
        } else {
          $file_macros{$inf} = [$func_name];
        }
        if(defined($macro_files{$func_name})){
          if(exists_in_array($inf,$macro_files{$func_name})){
            print STDERR "WARNING: Re-def of macro $func_name in the same file $inf assumed to be featurization\n";
          } else {
            push @{$macro_files{$func_name}}, $inf;
          }
        } else {
          $macro_files{$func_name} = [$inf];
        }

			next;
		}
	}
	close(IN);
}

sub parse_uses_file
{
  my $f = shift;
  # XXX IMPLEMENT!
}


sub is_func_def
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
		"long long int",
    "u64",
    "u32"
	);

	my @discreptors = (
		"",
		"static",
		"inline"
	);

  my @init_exit = (
    "",
    "__init",
    "__exit"
  );
	my $func;

	foreach my $key (@ret_types){
		foreach my $dis (@discreptors){
      foreach my $post_dis (@init_exit){
       my $match;
       if($dis eq ""){
         $match = "$key";
       } else {
         $match = "$dis\\s+$key";
       }
       if($post_dis ne ""){
        $match = $match . "\\s+$post_dis";
       }
  			if($l =~ /^\s*$match\s+(\S+)\s*\(/){
  				return $1;
  			}
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

sub is_func_use
{
  my $l = shift;
  # XXX Parse line... return list of functions used here. 
}

sub exists_in_array
{
  my $element = shift;
  my $arr_ref = shift;
  foreach my $sub (@$arr_ref){
    if($element eq $sub){
      return 1;
    }
  }
  return 0;
}
