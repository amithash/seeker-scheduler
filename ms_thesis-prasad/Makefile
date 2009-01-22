.SUFFIXES: .fig .eps .pdf .dot .dia 

DOCNAME = thesis

OTHERFILES = thesis.bib

TEXFILES=thesis.tex 

FIGFILES=

DOTFILES=

DIAFILES=

NOGENEPSFILES=

GENEPSFILES=$(FIGFILES:.fig=.eps) $(DOTFILES:.dot=.eps) $(DIAFILES:.dia=.eps)

EPSFILES=$(GENEPSFILES) $(NOGENEPSFILES) $(PYXEPSFILES)

PYXEPSFILES=

PDFFILES=$(EPSFILES:.eps=.pdf)

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
	pdflatex thesis.tex

thesis.dvi:$(TEXFILES) $(EPSFILES) $(OTHERFILES) $(FIGFILES)
	if [ ! -e thesis.bib ]; then ln -s ../../bib/thesis.bib; fi
	latex thesis 
	bibtex thesis || echo 'BiBTeX failed; continuing anyway'
	latex thesis
	latex thesis

spellcheck:
	for i in *.tex; do aspell --personal=./.spellme -c $$i; done

clean:
	rm -f $(PDFFILES) $(GENEPSFILES) $(PYXEPSFILES) figures/*.dvi figures/*.log \
		thesis.dvi thesis.aux thesis.bbl thesis.blg thesis.log thesis.ps thesis.pdf

thesis.bib:
	if [ ! -e thesis.bib ]; then ln -s ../../bib/thesis.bib; fi
