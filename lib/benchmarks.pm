package benchmarks;

my %bench_script = (
	"bzip2" => "401.bzip2.ref3.sh",
	"gobmk" => "445.gobmk.ref1.sh",
	"sjeng" => "458.sjeng.ref0.sh",
	"lbm" => "470.lbm.ref0.sh",
	"sphinx3" => "482.sphinx3.ref0.sh",
	"mcf" => "429.mcf.ref0.sh",
	"povray" => "453.povray.ref0.sh",
	"libquantum" => "462.libquantum.ref0.sh",
	"omnetpp" => "471.omnetpp.ref0.sh",
	"xalancbmk" => "483.xalancbmk.ref0.sh",
	"milc" => "433.milc.ref0.sh",
	"hmmer" => "456.hmmer.ref1.sh",
	"h264ref" => "464.h264ref.ref2.sh",
	"astar" => "473.astar.ref1.sh"
);

my %bench_bin = (
	"bzip2" => "401.bzip2",
	"gobmk" => "445.gobmk",
	"sjeng" => "458.sjeng",
	"lbm" => "470.lbm",
	"sphinx3" => "482.sphinx3",
	"mcf" => "429.mcf",
	"povray" => "453.povray",
	"libquantum" => "462.libquantum",
	"omnetpp" => "471.omnetpp",
	"xalancbmk" => "483.xalancbmk",
	"milc" => "433.milc",
	"hmmer" => "456.hmmer",
	"h264ref" => "464.h264ref",
	"astar" => "473.astar"
);

my %bin_bench = (
	"401.bzip2" => "bzip2",
	"445.gobmk" => "gobmk",
	"458.sjeng" => "sjeng",
	"470.lbm" => "lbm",
	"482.sphinx3" => "sphinx3",
	"429.mcf" => "mcf",
	"453.povray" => "povray",
	"462.libquantum" => "libquantum",
	"471.omnetpp" => "omnetpp",
	"483.xalancbmk" => "xalancbmk",
	"433.milc" => "milc",
	"456.hmmer" => "hmmer",
	"464.h264ref" => "h264ref",
	"473.astar" => "astar"
);

my $path = "$ENV{SEEKER_HOME}/AnalysisScripts/bench";

sub get_bench_name{
	my $bin = shift;
	if(defined($bin_bench{$bin})){
		return $bin_bench{$bin};
	} else {
		return "";
	}
}

sub get_binary_name{
	my $bench = shift;
	if(defined($bench_bin{$bench})){
		return $bench_bin{$bench};
	} else {
		return "";
	}
}

sub get_execute_script{
	my $in = shift;
	if(defined($bench_script{$in})){
		return $bench_script{$in};
	} else {
		return "";
	}
}

sub cleanup{
	my $bench_script_name = shift;
	my $bench_script_ex = get_execute_script($bench_script_name);
	open IN,"$path/cleanup/$bench_script_ex";
	my $a = join("",<IN>);
	close(IN);
	return $a;
}
sub setup{
	my $bench_script_name = shift;
	my $bench_script_ex = get_execute_script($bench_script_name);
	open IN,"$path/setup/$bench_script_ex";
	my $a = join("",<IN>);
	close(IN);
	return $a;
}
sub run{
	my $bench_script_name = shift;
	my $bench_script_ex = get_execute_script($bench_script_name);
	open IN,"$path/run/$bench_script_ex";
	my $a = <IN>;
	close(IN);
	chomp($a);
	return $a;
}


sub get_info{
	if(defined($bench_script{$in})){
		if($bench_script{$in} =~ /(.+\..+)\.ref(\d)/){
			my $bench_script_name = $1;
			my $ref = $2;
			return "$bench_script_name, reference input data: $ref, execute script: $bench_script{$in}";
		} 
	}
	return "";
}
true;

