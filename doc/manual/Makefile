all: pdf

dvi: cp-cmd-list.tex cp-directory-sample.tex
	latex manual
	latex manual

pdf: cp-cmd-list.tex cp-directory-sample.tex
	pdflatex manual
	pdflatex manual

.PHONY: cp-cmd-list.tex
cp-cmd-list.tex:
	./gen-docs.sh

.PHONY: cp-directory-sample.tex
cp-directory-sample.tex:
	(echo "\\begin{verbatim}"; cat ../../sys/hvf.directory; echo "\\end{verbatim}") > $@

clean:
	rm -f *.toc *.aux *.cb *.cb2 *.dvi *.log manual.pdf cp-cmd-list.tex cp-directory-sample.tex
	rm -rf tex/
