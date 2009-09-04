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
my %external_funcs;
my %call_made;

parse_def(@ARGV);

open OUT,"+>figure.dot" or die "Could not create figure.dot!\n";

# Print DOT header.
print OUT "digraph G {\n";

# Print Function structure.
foreach my $file (keys %files_all){
  if((not defined($file_funcs{$file})) and (not defined($file_macros{$file}))){
    next;
  }
  print OUT "/* $file */\n";
  print OUT "subgraph cluster_$files_all{$file} {\n";
  print OUT "label=\"$file\"\n";
  print OUT "shape=\"rectangle\"\n";
  if(defined($file_macros{$file})){
#    print OUT "subgraph cluster_$files_all{$file}_macro {\n";
#    print OUT "label=\"MACRO\"\n";
#    print OUT "shape=\"rectangle\"\n";
    my $ref = $file_macros{$file};
    foreach my $func (@$ref){
      print OUT "macro_$files_all{$file}_$func [label=\"$func\",color=\"lightgrey\",style=\"filled\"]\n";
    }
#    print OUT "}\n";
  }
  if(defined($file_funcs{$file})){
#    print OUT "subgraph cluster_$files_all{$file}_funcs {\n";
#    print OUT "label=\"FUNCTIONS\"\n";
#    print OUT "shape=\"rectangle\"\n";
    my $ref = $file_funcs{$file};
    foreach my $func (@$ref){
      print OUT "func_$files_all{$file}_$func [label=\"$func\"]\n";
    }
#    print OUT "}\n";
  }
  print OUT "}\n\n";
}

# Parse calls.
my $proc_call_string = parse_calls(@ARGV);
my @ext_funcs = keys %external_funcs;
if(scalar @ext_funcs > 0){
  print OUT "subgraph cluster_EXT_funcs {\n";
  print OUT "label=\"ExternalDependancies\"\n";
  print OUT "shape=\"rectangle\"\n";
  foreach my $ext (@ext_funcs){
    print OUT "EXT_$ext [label=\"$ext\"]\n";
  }
  print OUT "}\n";
}
print OUT "\n\n";
print OUT "$proc_call_string\n";

# Print calls.

# Print Footer.
print OUT "}\n";


# XXX Print the function structures based on files. 

# Everything is in %funcs and %macros. 
# XXX From here, as and when we find a use, we start adding an arrow. 
# Design descision, uses of functions not defined goes in a seperate DUMMY FILE called "ExternalDependancies"
# Make the ExternalDependancies structure "editable" so people can re-group them...
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
sub parse_def
{
  my @args = @_;
  foreach my $f (@args){
  	if(-e $f){
  		parse_def_file($f);
  	}
  	if(-d $f){
  		parse_def_dir($f);
  	}
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

sub parse_calls
{
  my @args = @_;
  my $call_string = "";
  foreach my $f (@args){
  	if(-e $f){
  		$call_string = parse_calls_file($f,$call_string);
  	}
  	if(-d $f){
  		$call_string = parse_calls_dir($f, $call_string);
  	}
  }
  return $call_string;
}

sub parse_calls_dir
{
	my $d = shift;
  my $call_string = shift;
	my @cont = <$d/*>;
	foreach my $f (@cont){
		if(-e $f){
			$call_string = parse_calls_file($f,$call_string);
		}
		if(-d $f){
			$call_string = parse_calls_dir($f,$call_string);
		}
	}
  return $call_string;
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

sub parse_calls_file
{
  my $f = shift;
  my $func_regexp = "[A-Za-z_0-9]+";
  my $call_string = shift;
  open IN, "$f" or die "Could not open $f\n";
  my $current_func = "";

  my $processing_func = 0;
  my $processing_macro = 0;
  my $indent_count = 0;
  my $at_least_one = 0;
  while(my $line = <IN>){
    chomp($line);
    my $func_name = is_func_def($line);
    if($f =~ /\.c$/){
      if($func_name ne "NULL"){
        $current_func = $func_name;
        $processing_func = 1;
        if($line =~ /{/){
          $indent_count++;
          $at_least_one = 1;
        }
        next;
      }
    }
    $func_name = is_macro($line);
    my $is_macro = 0;
    if($func_name ne "NULL"){
      $is_macro = 1;
      $current_func = $func_name;
      $processing_macro = 1;
      $processing_func = 0;
    }
    if($current_func eq ""){
      next;
    }
    my $bac_line = $line;

    if($processing_macro == 0 and $processing_func == 0){
      next;
    }
    
    # Parse the line
    while($line ne ""){
      if($line =~ /($func_regexp)\s*\((.+)$/){
        my $call = $1;
        my $rest = $2;
        if(is_not_keyword($call)){
          if($is_macro == 0 or ($is_macro == 1 and $call ne $current_func)){
            my $caller;
            if($processing_func == 1){
              $caller = "func_$files_all{$f}_$current_func";
            } else {
              $caller = "macro_$files_all{$f}_$current_func";
            }
            my $callee;
            if(defined($func_files{$call})){
              my $ref = $func_files{$call};
              foreach my $fc (@$ref){
                if(defined($func_files{$call})){
                  $callee = "func_$files_all{$fc}_$call";
                } elsif(defined($macro_files{$call})){
                  $callee = "macro_$files_all{$fc}_$call";
                } else{
                  print STDERR "ERROR: Unknown call $call\n";
                  next;
                }
                if(not defined($call_made{"$caller -> $callee"})){
                  $call_made{"$caller -> $callee"} = 1;
                  $call_string .= "$caller -> $callee\n";
                }
              }
            } else {
              $callee = "EXT_$call";
              if(not defined($call_made{"$caller -> $callee"})){
                $call_string .= "$caller -> $callee\n";
                $external_funcs{$call} = 1;
                $call_made{"$caller -> $callee"} = 1;
              }
            }
          }

        }
        $line = $rest;
      } else {
        $line = "";
      }
    }

    if($processing_func == 1){
      if($at_least_one == 0){
        if($bac_line =~ /{/){
          $at_least_one = 1;
          $indent_count++;
        }
      }
      if($bac_line =~ /{/){
        $indent_count++;
      }
      if($bac_line =~ /}/){
        $indent_count--;
      }
      if($indent_count <= 0){
        $processing_func = 0;
        $current_func = "";
      }
    }

    if($processing_macro == 1){
      if($bac_line =~ /\\\s*$/){
        $processing_macro = 1;
      } else {
        $processing_macro = 0;
      }
    }

  }
  close(IN);

  return $call_string;
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

sub is_not_keyword
{
  my $func = shift;
  my %keywords = (
    "for" => 1,
    "if" => 1,
    "while" => 1,
    "sizeof" => 1,
    "switch" => 1
  );
  if(defined($keywords{$func})){
    return 0;
  }
  return 1;
}
