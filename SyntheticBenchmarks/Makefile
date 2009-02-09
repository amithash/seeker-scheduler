all: synth_bench

CACHE_SIZE=$(shell perl -e 'my $$a = `cat /proc/cpuinfo | grep "cache size"`;$$a =~ /(\d+)/; print $$1')

synth_bench:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DCACHE_SIZE="$(CACHE_SIZE).0"
clean:
	rm -f synth_bench
