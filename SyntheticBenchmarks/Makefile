all: synth_bench

MEM=$(shell perl -e 'my $$a = `cat /proc/meminfo | grep MemTotal`;$$a =~ /(\d+)/; print $$1')

synth_bench:
	gcc -fmerge-all-constants synth_bench.c -o synth_bench -lm -DMEM_SIZE="$(MEM).0"
clean:
	rm -f synth_bench
