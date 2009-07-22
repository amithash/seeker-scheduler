.SUFFIXES: .fig .eps .pdf .dot .dia 

DOCNAME = thesis

OTHERFILES = thesis.bib

GENLINES=python ./gen_lines.py
GENLINESCACHE=python ./gen_cache_lines.py
GENBARS=python ${CAR_PAPER_SCRIPTS}/gen_bars.py
GEOMEAN=perl ${CAR_PAPER_SCRIPTS}/compute_geomean.pl
STACKIFY=perl ${CAR_PAPER_SCRIPTS}/stackify.p

TEXFILES=thesis.tex 

FIGFILES=

DOTFILES=

DIAFILES=

NOGENPDFFILES=

GENEPSFILES=$(FIGFILES:.fig=.eps)  $(DIAFILES:.dia=.eps)

GENEPSFILES=$(FIGFILES:.fig=.eps) \
            $(GENFIGFILES:.fig=.eps) \
	        $(DATAFILES:.data=.eps) \
			$(DOTFILES:.dot=.eps) 

EPSFILES=$(GENEPSFILES) $(NOGENEPSFILES) $(PYXEPSFILES)

PYXEPSFILES=

PDFFILES=$(EPSFILES:.eps=.pdf) $(PSFILES:.ps=.pdf)

.fig.eps:
	fig2dev -L eps $< $@

.dot.eps:
	dot -Tps2 -o $@.bak $<
	eps2eps $@.bak $@
	rm $@.bak 

.dia.eps:
	dia -n -t eps -e $@ $<

.eps.pdf:
	epstopdf $<

.ps.pdf:
	pstopdf $<

pdf: thesis.pdf

ps: thesis.ps

print: thesis.pdf
	lpr thesis.pdf

thesis.ps: thesis.dvi
	dvips -o thesis.ps thesis.dvi

thesis.pdf: $(TEXFILES) $(EPSFILES) $(PDFFILES) $(OTHERFILES)
	if [ ! -e thesis.bib ]; then ln -s ../../bib/thesis.bib; fi
	pdflatex thesis.tex
	bibtex thesis || echo 'BiBTeX failed; continuing anyway'
	pdflatex thesis.tex
	pdflatex thesis.tex | grep -i "warning" > warning.log

thesis.dvi:$(TEXFILES) $(EPSFILES) $(OTHERFILES) $(FIGFILES)
	if [ ! -e thesis.bib ]; then ln -s ../../bib/thesis.bib; fi
	latex thesis 
	bibtex thesis || echo 'BiBTeX failed; continuing anyway'
	latex thesis
	latex thesis | grep -i "Warning" > warning.log

clean:
	rm -f $(PDFFILES) $(GENEPSFILES) $(PYXEPSFILES) figures/*.dvi figures/*.log \
		thesis.dvi thesis.aux thesis.bbl thesis.blg thesis.log thesis.ps thesis.pdf \
		thesis.toc thesis.loc thesis.lof thesis.lot warning.log

thesis.bib:
	if [ ! -e thesis.bib ]; then ln -s ../../bib/thesis.bib; fi


figures/memory_bound.eps: figures/%.eps: data/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Tuple Count" --yend 4000000000 --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-r" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5 --fillpatterns "color.grey.black,color.grey.white,color.grey(.3),color.grey(.6)" --output  $@ $<;

figures/compression_chart.eps: figures/%.eps: data/%.data
	${GENLINES} figures/compression_chart.eps data/compression_chart.data

figures/dinpc_varorder_mem.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Byte" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --output  $@ $<;

figures/dinpc_varorder_time.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Second" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5 --output  $@ $<;

figures/dindin_varorder_mem.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Byte" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --output  $@ $<;

# figures/dindin_varorder_mem.eps : figures/%.eps: data/bddcon/%.data
# 	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Byte" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/dindin_varorder_time.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Second" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --output  $@ $<;

figures/dinrdy_varorder_mem.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --yend 0.75 --ytitle "Inst. Per Byte" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-r" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --output  $@ $<;

figures/dinrdy_varorder_time.eps : figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Inst. Per Second" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --output  $@ $<;

figures/300twolf_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/300twolf_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/300twolf_cache_conflictssec.eps temp.data
	rm temp.data

figures/254gap_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/254gap_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/254gap_cache_conflictssec.eps temp.data
	rm temp.data

figures/197parser_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/197parser_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/197parser_cache_conflictssec.eps temp.data
	rm temp.data

figures/186crafty_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/186crafty_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/186crafty_cache_conflictssec.eps temp.data
	rm temp.data

figures/181mcf_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/181mcf_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/181mcf_cache_conflictssec.eps temp.data
	rm temp.data

figures/176gcc_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/176gcc_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/176gcc_cache_conflictssec.eps temp.data
	rm temp.data

figures/175vpr_cache_conflictssec.eps: figures/%.eps: data/bddcon/%.csv
	python ./comma2space.py data/bddcon/175vpr_cache_conflictssec.csv temp.data
	${GENLINESCACHE} figures/175vpr_cache_conflictssec.eps temp.data
	rm temp.data

figures/memory_time.eps: figures/%.eps: data/bddcon/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-r" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_250mil_64res_times.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 5 --ytitle "Performance Improvement" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_250mil_131072res_times.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 5 --ytitle "Performance Improvement" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/254gap500milTimes.eps : figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Resolution" --ytitle "Times Improvement" --ysplit [[0,150],[1300,1500]] --ylabels "75" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_gentimes_100mil.eps : figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Seconds" --ysplit [[0,200],[1000,4000]] --ylabels "100" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;	

figures/graph_gentimes_50mil.eps : figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Seconds" --ysplit [[0,21],[400,1600]] --ylabels "50" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;	

figures/graph_gentimes_25mil.eps :  figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Seconds" --ysplit [[0,20],[200,600]] --ylabels "20" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;	

figures/graph_gentimes_15mil.eps : figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --ytitle "Seconds" --ylabels "300,200,10" --ysplit [[0,10],[100,300]] --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;	

figures/graph_gentimes_100milsubgraph.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 90 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_100mil_parameterwarm.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 0.5 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_100mil_parameterwarm_render.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 5 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/graph_100mil_adamantium.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 80 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/254gap100milParaMeter.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Resolution" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 20 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/254gap100milAdamantium.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Resolution" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 3500 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/254gap500milParaMeter.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Resolution" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 80 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/254gap500milAdamantium.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Resolution" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 500 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/subgraph100mil_parameterwarm.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 0.001 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/subgraph100mil_times.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --height 5 --ytitle "Times Improvement" --nolegend --width 13 --yend 700000 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/subgraph100mil_parametercold.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 90 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/subgraph100mil_adamantium.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --xtitle "Benchmark" --height 5 --ytitle "Seconds" --legenddist 0.05 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 80 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/dinID.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 4 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 0.0005 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

figures/1000deepslice.eps: figures/%.eps: data/parameter/%.data
	${GENBARS} --nolegend --xtitle "Benchmark" --height 4 --ytitle "Seconds" --legenddist 0.1 --legendxshift +0.2 --legendpos "-t-l" --legendyshift +0.2 --width 13 --yend 0.03 --delim ',' --subnamesize .5  --fillpatterns "color.grey.black,color.grey.white,color.grey(.6)" --output  $@ $<;

spellcheck:
	for i in $(TEXFILES); do if [ $$i == paper.tex ]; then continue; fi; aspell --personal=./personal.wl -c $$i; done
