#*************************************************************************
# Copyright 2009 Amithash Prasad                                         *
#                                                                        *
# This file is part of Seeker                                            *
#                                                                        *
# Seeker is free software: you can redistribute it and/or modify         *
# it under the terms of the GNU General Public License as published by   *
# the Free Software Foundation, either version 3 of the License, or      *
# (at your option) any later version.                                    *
#                                                                        *
# This program is distributed in the hope that it will be useful,        *
# but WITHOUT ANY WARRANTY; without even the implied warranty of         *
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
# GNU General Public License for more details.                           *
#                                                                        *
# You should have received a copy of the GNU General Public License      *
# along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
#*************************************************************************


package seeker;

sub get_dir_tree{
	my $path = shift;
	open LS, "ls $path |";
	my @tree = split(/\n/, join("", <LS>));
	close(LS);
	return @tree;
}

sub do_per_dir{
	my $dir = shift;
	my $mother = shift;
	my $out = shift;
	my $operation = shift;
	my @dir_tree = get_dir_tree($dir);
	foreach my $file (@dir_tree){
		if(-d "$dir/$file"){
			do_per_dir("$dir/$file", $mother, "$out/$file", $operation);
		}
		else{
			my $relative = extract_relative($mother,$dir);
			make_dir("$out/$relative");
			my $oper = $operation;
			$oper =~ s/INPUT/$dir\/$file/;
			$oper =~ s/OUTPUT/$out\/$file/;
			system("$oper");
		}
	}
}

sub extract_relative{
	my $in1 = shift;
	my $in2 = shift;
	my @sin1 = split(/\//, $in1);
	my @sin2 = split(/\//, $in2);
	my $ret = join("/", @in2[($#in1+1)..$#in2]);
	return $ret;
}

sub make_dir{
	my $path = shift;
	if(-d "$path"){
		return;
	}
	elsif(-d "$one_up_path"){
		my @spath = split(/\//, $path);
		my $one_up_path = join("/", @spath[0..($#spath-1)]);
		my $dir = $spath[$#spath];
		system("mkdir $path");
	}
	else{
		my @spath = split(/\//, $path);
		my $one_up_path = join("/", @spath[0..($#spath-1)]);
		my $dir = $spath[$#spath];
		make_dir($one_up_path);
		system("mkdir $path");
	}
}

true;

