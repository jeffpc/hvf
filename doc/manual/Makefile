all: pdf

dvi: cp-cmd-list.tex
	latex manual
	latex manual

pdf: cp-cmd-list.tex
	pdflatex manual
	pdflatex manual

.PHONY: cp-cmd-list.tex
cp-cmd-list.tex:
	./gen-docs.sh

clean:
	rm -f *.toc *.aux *.cb *.cb2 *.dvi *.log manual.pdf cp-cmd-list.tex
	rm -rf tex/
