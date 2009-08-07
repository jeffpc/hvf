all: pdf

dvi: cp-cmd-list.tex
	latex manual

pdf: cp-cmd-list.tex
	pdflatex manual

.PHONY: cp-cmd-list.tex
cp-cmd-list.tex:
	./gen-docs.sh

clean:
	rm -f manual.aux manual.cb manual.cb2 manual.dvi manual.log manual.pdf
