#!/usr/bin/octave

my_argv = argv();
my_argc = size(my_argv);
if my_argc != 3
	printf("Usage: %s DataDir PlotsDir Interval\n",program_name);
	exit();
end

% Plots everything bitches!

% '\' for window, '/' for linux
dirsep = filesep;

% Image type: jpeg or png
img = 'jpg';

% Path to the dir containing the in data tree
inbase = deblank(cell2mat(my_argv(1,:)));
outbase = deblank(cell2mat(my_argv(2,:)));
interval = cell2mat(my_argv(3,:));
% Check input data dir.
if isdir(inbase) == 0
	printf("%s does not exist\n",inbase);
	exit();
end

if mkdir(outbase) == 1
	printf("%s did not exist, creating it\n",outbase);
end

outbase_mutate = strcat(outbase,dirsep,'mutate_plots');
outbase_bench =  strcat(outbase,dirsep,'bench_plots');

% Do not overwrite existing plots. Spit an error and exit.
if isdir(outbase_mutate) == 1
	printf("%s exists in your output dir. Please back it up\n",outbase_mutate);
	exit();
end
mkdir(outbase_mutate);

if isdir(outbase_bench) == 1
	printf("%s exists in your output dir. Please back it up\n",outbase_bench);
	exit();
end
mkdir(outbase_bench);

% The type array
type = strvcat('default','alt1','alt2');

% The delta array
delta = strvcat('delta_1', 'delta_2','delta_4','delta_8','delta_16');

% The benchmarks array

% Currently perlbench and gcc are not working right... so till I get it to work,
% do not generate any graphs for it.
%bench = strvcat('400.perlbench.tsv', '403.gcc.tsv', '445.gobmk.tsv', '458.sjeng.tsv', ...
%      		'464.h264ref.tsv', '473.astar.tsv', '401.bzip2.tsv', '429.mcf.tsv', ...
%		'456.hmmer.tsv', '462.libquantum.tsv', '471.omnetpp.tsv', '483.xalancbmk.tsv');
%bench = strvcat('445.gobmk', '458.sjeng', ... 
%      	'464.h264ref', '473.astar', '401.bzip2', '429.mcf', ...
%	'456.hmmer', '462.libquantum', '471.omnetpp', '483.xalancbmk');
load('benchlist');

energy = zeros(size(type,1)+2,size(delta,1));
% Plot all.
fraction_done = 0;
inp0 = strcat(inbase,dirsep,'power_0');
inp1 = strcat(inbase,dirsep,'power_1');
pr0 = load(inp0);
pr1 = load(inp1);
t0 = pr0(:,1);
p0 = pr0(:,2);
t1 = pr1(:,1);
p1 = pr1(:,2);
outp = strcat(outbase,dirsep,'base.jpg');
newp1 = figure;
set (newp1, "visible", "off");
subplot(2,1,1);
plot(t0,p0,'b');
xlabel('Time (Seconds)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('Power State 0 (Watts)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
a0 = trapz(t0,p0);
subplot(2,1,2);
plot(t1,p1,'r');
xlabel('Time (Seconds)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
ylabel('Power State 1 (Watts)','FontName', 'DejaVuSans-Bold.ttf', 'FontSize', 8);
a1 = trapz(t1,p1);
print('new1','-djpeg',outp);
close all;

for ii = 1:size(type,1)
	type_i = deblank(type(ii,:));
	mkdir(strcat(outbase_mutate,dirsep,type_i));
	mkdir(strcat(outbase_bench,dirsep,type_i));
	for jj = 1:size(delta,1)
		delta_i = deblank(delta(jj,:));
		inpath1 = strcat(inbase,dirsep,type_i,dirsep,delta_i,dirsep,'MUTATE_REQUESTED');
		inpath2 = strcat(inbase,dirsep,type_i,dirsep,delta_i,dirsep,'MUTATE_GIVEN');
		inpath3 = strcat(inbase,dirsep,type_i,dirsep,delta_i,dirsep,'PPROFILE');
		outpath1 = strcat(outbase_mutate,dirsep,type_i,dirsep,delta_i,'.',img);
		energy(ii,jj) = plot_mutate(inpath1,inpath2,inpath3,outpath1,img,interval);

		for kk = 1:size(bench,1)
			bench_i = deblank(bench(kk,:));
			bench_dir = strcat(outbase_bench,dirsep,type_i,dirsep,bench_i);
			if(isdir(bench_dir) == 0)
				mkdir(bench_dir);
			end
			inpath = strcat(inbase,dirsep,type_i,dirsep,delta_i,dirsep,'sch',dirsep,bench_i,'.tsv');
			outpath2 = strcat(bench_dir,dirsep,delta_i,'.',img);
			outpath3 = strcat(bench_dir,dirsep,delta_i,'_hist.',img);
			plot_bench(inpath,outpath2,outpath3,img);
		end
	end
	fraction_done = fraction_done + (100.0/size(type,1));
	printf("%f done\n",fraction_done);
end
for ii = 1:size(delta,1)
	energy(size(type,1)+1,ii) = a0;
	energy(size(type,1)+2,ii) = a1;
end

outtable = strcat(outbase,dirsep,'energy');
save('-ascii',outtable,'energy');


