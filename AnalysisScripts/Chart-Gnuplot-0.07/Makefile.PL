use ExtUtils::MakeMaker;

WriteMakefile(
	AUTHOR        => 'Ka-Wai Mak',
	NAME          => 'Chart::Gnuplot',
	VERSION_FROM  => 'lib/Chart/Gnuplot.pm',
	ABSTRACT_FROM => "lib/Chart/Gnuplot.pm",
	PREREQ_PM     => { File::Copy => 2.08, File::Temp => 0.14 },
	dist          => {
		SUFFIX   => '.gz',
		COMPRESS => 'gzip -f'
	}
);
