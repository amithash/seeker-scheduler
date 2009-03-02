all: synth_bench
debug: synth_bench_debug

CACHE_SIZE=$(shell perl -e 'my $$a = `cat /proc/cpuinfo | grep "cache size"`;$$a =~ /(\d+)/; print $$1')

synth_bench:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DCACHE_SIZE="$(CACHE_SIZE).0"
synth_bench_debug:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DCACHE_SIZE="$(CACHE_SIZE).0" -DDEBUG
clean:
	rm -f synth_bench
