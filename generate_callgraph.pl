#!/usr/bin/perl

###########################################################################
#    This program is free software: you can redistribute it and/or modify #
#    it under the terms of the GNU General Public License as published by #
#    the Free Software Foundation, either version 3 of the License, or    #
#    (at your option) any later version.                                  #
#                                                                         #
#    This program is distributed in the hope that it will be useful,      #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of       #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        #
#    GNU General Public License for more details.                         #
#                                                                         #
#    You should have received a copy of the GNU General Public License    #
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.#
###########################################################################

use strict;
use warnings;
use Getopt::Long;


################################################################
# Parameters
################################################################

my $cluster = 1;
my $rankdir = "LR";
my $file_shape = "rectangle";
my $macro_col = "black";
my $macro_fill_col = "lightgrey";
my $macro_style = "filled";
my $func_col = "black";
my $func_fill_col = "white";
my $func_style = "filled";
my $ext_col = "black";
my $ext_fill_col = "green";
my $ext_style = "filled";
my $name = "figure";
my $type = "svg";
my $dot_app = "dot";
my $ignore_ext = 0;
my $cluster_ext = 1;
my $cluster_macro = 1;
my $cluster_func = 1;
my $help = 0;

GetOptions(
	"cluster!" => \$cluster,
	"rankdir=s" => \$rankdir,
	"macrocol=s" => \$macro_fill_col,
	"funccol=s" => \$func_fill_col,
	"extcol=s" => \$ext_fill_col,
	"name=s"    => \$name,
	"type=s"    => \$type,
	"app=s"   => \$dot_app,
  "ignore_ext" => \$ignore_ext,
  "cluster_ext" => \$cluster_ext,
  "cluster_macro" => \$cluster_macro,
  "cluster_func" => \$cluster_func,
	"help" => \$help
);

my $figure_name = "$name.dot";
my $out_name = "$name.$type";
my $calls_name = "$name.calls";

my %apps = (
	"dot" => 1,
	"neato" => 1,
	"twopi" => 1,
	"circo" => 1,
	"fdp" => 1
);

my %formats = (
	"ps" => 1,
	"svg" => 1,
	"svgz" => 1,
	"fig" => 1,
	"mif" => 1,
	"hpgl" => 1, 
	"pcl" => 1,
	"png" => 1, 
	"gif" => 1,
	"dia" => 1, 
	"imap" => 1,
	"cmapx" => 1  
);

if($help == 1){
	usage();
	exit;
}

if(not defined($apps{$dot_app})){
	print "Unsupported App: $dot_app. Avaliable app: ";
	foreach my $app (sort keys %apps){
		print "$app ";
	}
	print "\n";
}
if(not defined($formats{$type})){
	print "Unsupported format: $type. Avaliable formats: ";
	foreach my $format (sort keys %formats){
		print "$format ";
	}
	print "\n";
}

if($#ARGV < 0){
  print "\nERROR: No arguments.\n\n";
  usage();
  exit;
}

if($cluster == 0){
  $cluster_ext = 0;
  $cluster_macro = 0;
  $cluster_func = 0;
}


################################################################
# GLOBAL VARS (ALL SUBS)
################################################################

# Used by remove_comment();
my $comment_block_start = 0; 

# $file_to_func{file} = {func1 => [calls1], func2 => [calls2]};
my %file_to_func;

# $func_to_file{func} = {file1 => 1, file2 => 1};
my %func_to_file;

# $file_to_macro{file} = {macro1 => [calls1], macro2 => [calls2]};
my %file_to_macro;

# $macro_to_file{func} = {macro1 => 1, macro2 => 1};
my %macro_to_file;

# Mapping from file to count; Updated by get_all_files.
my $file_count = 0;
my %processed_files;

# a func or macro if internal will be defined.
my %internal;
my %external;

my %call_made;

################################################################

main(@ARGV);

sub main
{
	my @args = @_;
	print STDERR "Parsing files\n";
	my @files = get_all_files(@args);
	foreach my $file (@files){
		parse_file($file);
	}

	print "-------- parse complete ------------\n";

	open CALLS, "+>$calls_name" or die "could not create $calls_name\n";

	# Print them to stdout.
	foreach my $file (sort keys %processed_files){
		print CALLS "$file\n";
		if(defined($file_to_macro{$file})){
			print CALLS "\tMACROS:\n";
			my %macros = %{$file_to_macro{$file}};
			foreach my $macro (sort keys %macros){
				print CALLS "\t\t$macro\n";
				my %calls = %{$macros{$macro}};
				foreach my $call (sort keys %calls){
					if(not defined($internal{$call})){
						$external{$call} = 1;
					}
          if($ignore_ext == 1){
            if(defined($internal{$call})){
					    print CALLS "\t\t\t$call\n";
            }
          } else {
					  print CALLS "\t\t\t$call\n";
          }
				}
				
			}
		}
		if(defined($file_to_func{$file})){
			print CALLS "\tFUNCS:\n";
			my %funcs = %{$file_to_func{$file}};
			foreach my $func (sort keys %funcs){
				print CALLS "\t\t$func\n";
				my %calls = %{$funcs{$func}};
				foreach my $call (sort keys %calls){
					if(not defined($internal{$call})){
						$external{$call} = 1;
					}
          if($ignore_ext == 1){
            if(defined($internal{$call})){
					    print CALLS "\t\t\t$call\n";
            }
          } else {
					  print CALLS "\t\t\t$call\n";
          }
				}
				
			}
		
		}
	}
	print "-------- printing to calls file complete ------------\n";
	print_to_dot();
	print "-------- Executing $dot_app ------------\n";

	system("$dot_app -T$type $figure_name -o $out_name");
}

# take an array of files and dirs, return a list of c files
sub get_all_files
{
	my @fs = @_;
	my @ret_files = ();

	foreach my $f (@fs){
		if(-d $f){
			my @tmp1 = <$f/*>;
			my @tmp = get_all_files(@tmp1);
			foreach my $t (@tmp){
				push @ret_files, $t;
			}
		} elsif(-e $f){
			$processed_files{$f} = $file_count;
			$file_count++;

			if($f =~ /\.c$/){
				push @ret_files, $f;
			} elsif($f =~ /\.h$/){
				push @ret_files, $f;
			}
		} 
	}
	return @ret_files;
}

sub parse_file
{
	my $file = shift;

	print STDERR "Parsing file: $file\n";

	open IN, "$file" or die "Could not open $ARGV[0]!\n";

	my $processing_macro = 0;
	my $processing_macro_name = "";

	my $processing_func = 0;
	my $processing_func_name = "";
	my $indent_count = 0;

	while(my $line = <IN>){
		chomp($line);

		my $l = remove_comment($line);
		if($l eq ""){
			next;
		}
		my ($macro_name,$m_remaining) = is_macro($l);
		if($macro_name ne ""){
			# Process macro
			my @list = get_calls($m_remaining);
			add_to_macro($macro_name,$file,\@list);
			if($l =~ /\\$/){
				# Setup to continue processing at next line.
				$processing_macro = 1;
				$processing_macro_name = $macro_name;
			}
			next;
		}

		if($processing_macro == 1){
			my @list = get_calls($l);
			add_to_macro($processing_macro_name,$file,\@list);
			if($l !~ /\\\s*$/){
				$processing_macro = 0;
				$processing_macro_name = "";
			}
			next;
		}

		my $func_name = is_func($l);
		if($func_name ne ""){
			$processing_func_name = $func_name;
			$processing_func = 0;
			$indent_count = 0;
			add_to_func($processing_func_name, $file, []);
		}

		# Function call has not started yet. Wait for
		# opening brace.
		if($processing_func_name ne "" and $processing_func == 0){
			my $open = count_char("{",$l);
			my $close = count_char("}",$l);

			# Positive if more open braces.
			my $overflow = $open - $close;

			if($overflow > 0){
				$processing_func = 1;
				$indent_count += $overflow;
			}
			next;
		}

		if($processing_func == 1){
			my @list = get_calls($l);

			add_to_func($processing_func_name, $file, \@list);

			my $open = count_char("{",$l);
			my $close = count_char("}",$l);
			my $overflow = $open - $close;
			$indent_count += $overflow;
			if($indent_count <= 0){
				$indent_count = 0;
				$processing_func = 0;
				$processing_func_name = "";
			}
		}
	}
}

sub add_to_func
{
	my $name = shift;
	my $file = shift;
	my $list_ref = shift;
	if(not defined($file_to_func{$file})){
		$file_to_func{$file} = {};
	}
	if(not defined($file_to_func{$file}->{$name})){
		$file_to_func{$file}->{$name} = {};
	}
	foreach my $call (@$list_ref){
		$file_to_func{$file}->{$name}->{$call} = 1;
	}
	if(not defined($func_to_file{$name})){
		$func_to_file{$name} = {};
	}
	if(not defined($func_to_file{$name}->{$file})){
		$func_to_file{$name}->{$file} = 1;
	}
	$internal{$name} = 1;
}
sub add_to_macro
{
	my $name = shift;
	my $file = shift;
	my $list_ref = shift;
	if(not defined($file_to_macro{$file})){
		$file_to_macro{$file} = {};
	}
	if(not defined($file_to_macro{$file}->{$name})){
		$file_to_macro{$file}->{$name} = {};
	}
	foreach my $call (@$list_ref){
		$file_to_macro{$file}->{$name}->{$call} = 1;
	}
	if(not defined($macro_to_file{$name})){
		$macro_to_file{$name} = {};
	}
	if(not defined($macro_to_file{$name}->{$file})){
		$macro_to_file{$name}->{$file} = 1;
	}
	$internal{$name} = 1;
}

sub count_char
{
    my $char = shift;
    my $line = shift;
    my $count = 0;
    my @arr = split(//,$line);
    foreach my $ch (@arr){
        if($ch eq $char){
            $count++;
        }
    }
    return $count;
}

sub get_calls
{
	my $line = shift;
	my $func_regexp = "[A-Za-z_0-9][A-Za-z_0-9]+";

	my @call_list = ();

	while($line ne ""){
		if($line =~ /($func_regexp)\s*\((.+)$/){
			my $call = $1;
			my $rest = $2;
			
			if(is_keyword($call) == 0){
				push @call_list,$call;
			}
			$line = $rest;
		} else {
			$line = "";
		}
	}
	return @call_list;
}

sub is_keyword
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
        return 1;
    }
    return 0;
}

sub is_macro
{
    my $l = shift;
    my $macro;
    if($l =~/^\s*#define\s+([\d\S]+)\((.+)/){
        return ($1,$2);
    }
    return ("","");
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
    if($l =~ /;\s*$/){
        return "";
    }

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
    return "";
}


sub remove_comment
{
	my $l = shift;
	my $line = $l;

	if($comment_block_start == 1){
		if($line =~ /\*\/\s*$/){
			$comment_block_start = 0;
			return "";
		} elsif($line =~ /\*\/(.+)$/){
			$comment_block_start = 0;
			$l = $1;
		} else {
			return "";
		}
	} else {
		if($line =~ /^\s*\/\*/){
			$comment_block_start = 1;
			if($line =~ /\*\/\s*$/){
				$comment_block_start = 0;
				return "";
			} elsif($line =~/\*\/(.+)$/){
				$l = $1;
				$comment_block_start = 0;
			} else {
				return "";
			}
		} elsif($line =~ /^(.+)\/\*/){
			$l = $1;
			$comment_block_start = 1;
			if($l =~ /\*\/(.+)$/){
				$l .= $1;
				$comment_block_start = 0;
			}
		}
	}

	if($l =~ /^(.*)\/\//){
		$l = $1;
	}
	if($l =~/^\s*$/){
		return "";
	}
	return $l;
}

sub print_to_dot
{
	open OUT, "+>$figure_name" or die "Could not write to $figure_name!\n";


	# HEADER
	print OUT "digraph all {\n";
	print OUT "\trankdir=$rankdir\n";
	
	# DRAW FILE FUNCTION/MACRO NODES
	foreach my $file (sort keys %processed_files){
		# THis file has nothing! Skip it!
		if((not defined($file_to_func{$file})) and (not defined($file_to_macro{$file}))){
			next;
		}
		if($cluster == 1){
			print OUT "/* $file */\n";
			print OUT "\tsubgraph cluster_$processed_files{$file} {\n";
		        print OUT "\t\tlabel=\"$file\"\n";
			print OUT "\t\tshape=\"$file_shape\"\n";
    }
		if(defined($file_to_macro{$file})){
			foreach my $macro (sort keys %{$file_to_macro{$file}}){
				print OUT "\t\tmacro_$processed_files{$file}_$macro [label=\"$macro\",color=\"$macro_col\",fillcolor=\"$macro_fill_col\",style=\"$macro_style\"]\n";
			}
		}
		if(defined($file_to_func{$file})){
			foreach my $func (sort keys %{$file_to_func{$file}}){
				print OUT "\t\tfunc_$processed_files{$file}_$func [label=\"$func\",color=\"$func_col\",fillcolor=\"$func_fill_col\",style=\"$func_style\"]\n";
			}
		}
		if($cluster == 1){
			print OUT "\t}\n\n";
		}
	}
  if($ignore_ext != 1){
  	# DRAW EXTERNAL NODES
  	if(scalar (keys %external) > 0){
  		if($cluster_ext == 1){
  			print OUT "\tsubgraph cluster_EXT {\n";
  			print OUT "\t\tlabel=\"ExternalDependancies\"\n";
  			print OUT "\t\tshape=\"$file_shape\"\n";
  		}
  		foreach my $func (sort keys %external){
  			print OUT "\t\tfunc_EXT_$func [label=\"$func\",color=\"$ext_col\",fillcolor=\"$ext_fill_col\",style=\"$ext_style\"]\n"
  		}
  		if($cluster_ext == 1){
  			print OUT "\t}\n\n";
  		}
  	} 
  }

	# DRAW ARCS
	foreach my $func (sort keys %func_to_file){
		foreach my $file (sort keys %{$func_to_file{$func}}){
			draw_arcs(\%file_to_func,$file,$func,"func");
		}
	}
	foreach my $macro (sort keys %macro_to_file){
		foreach my $file (sort keys %{$macro_to_file{$macro}}){
			draw_arcs(\%file_to_macro,$file,$macro,"macro");
		}
	}
	
	# DRAW FOOTER
	print OUT "}\n";
}
sub draw_arcs
{
	my $file_to_ref = shift;
	my $file = shift;
	my $func = shift;
	my $caller_desc = shift;
	my $caller = "${caller_desc}_$processed_files{$file}_$func";
	foreach my $call (sort keys %{$file_to_ref->{$file}->{$func}}){
		if(defined($internal{$call})){
			if(defined($func_to_file{$call})){
				foreach my $file (sort keys %{$func_to_file{$call}}){
					my $callee = "func_$processed_files{$file}_$call";
					my $line = "$caller -> $callee";
					if(not defined($call_made{$line})){
						$call_made{$line} = 1;
						print OUT "\t$line\n";
					}
				}
			} elsif(defined($macro_to_file{$call})) {
				foreach my $file (sort keys %{$macro_to_file{$call}}){
					my $callee = "macro_$processed_files{$file}_$call";
					my $line = "$caller -> $callee";
					if(not defined($call_made{$line})){
						$call_made{$line} = 1;
						print OUT "\t$line\n";
					}
				}
			} else {
				print STDERR "SOMETHING IS WRONG. Not external, but not defined either!\n";
			}
		} elsif($ignore_ext == 0){
			my $callee = "func_EXT_$call";
			my $line = "$caller -> $callee";
			if(not defined($call_made{$line})){
				print OUT "\t$line\n";
				$call_made{$line} = 1;
			}
		}
	}
}

sub usage 
{
	print "USAGE: $0 OPTIONS <file list>\n";
  print "
    OPTIONS:
      --cluster (default) - Cluster function definitions based on files in graphs,
      --nocluster - Do not cluster function definitions. Allows dot to organize on its own.

      --rankdir=RANK - this is the graph direction
                       SUPPORTED: LR, UP, BU - Refer to graphviz documentation

      --macrocol=COL - color of the nodes representing macro definitions. (default=grey)
      --funccol=COL  - color of the nodes representing function definitions. (default=white)
      --extcol=COL   - color of the nodes representing external function definition dependancies (default=green)

      --name=PATH    - Path and name of the output project. There will be 3 files generated. (default: figure)
                       PATH.calls - Contains ascii representation of the calls made by each definition.
                       PATH.dot   - The DOT graph file. 
                       PATH.\$type- The graph plotted using --app with type being from --type (See below)
      --type=TYPE    - The type of the output generated file using dot. (default: svg)
                       Supported values: ps,svg,svgz,fig,mif,hpgl,pcl,png,gif,dia,imap,cmapx

      --app=APP      - The graphviz application to use (default: dot)
                       Supported values: dot, neato, twopi, circo, fdp

      --help         - Shows this text

      <file list>
      path to a directory or list of dirs/c-files. Note that only *.c or *.h files will be processed. 
      There is NO support for C++. 

      (C) Amithash Prasad 2009
      See terms of GPL v3 refer to:
      http://www.gnu.org/licenses/gpl-3.0-standalone.html
  ";

}
