write_docs()
{
cat "$1" | awk -v fmt="$2" '
BEGIN{
	docdir = ".";

	fname = "";
}

/^[/ ]\*!!! / {
	# begin a new command
	if (fname != "")
		close(fname);

	name="";
	fname="";
	for(i=2; i<=NF; i+=1) {
		name = name $i " ";
		fname = fname $i "-";
	}
	name = substr(name, 1, length(name)-1);
	_fname = docdir "/" fmt "/" substr(fname, 1, length(fname)-1)
	fname = _fname "." fmt;

	print "Writing out ", fname;

	print "\\input{" _fname "}" >> "cp-cmd-list.tex"

	print "\\section*{" name "}" > fname;
	mode = "none";
}

/^[/ ]\*!! SYNTAX$/ {
	mode = "syntax";
	print "\\begin{syntdiag}" >> fname;
	next;
}

/^[/ ]\*!! XATNYS$/ {
	mode = "none";
	print "\\end{syntdiag}" >> fname;
	next;
}

/^[/ ]\*!! AUTH / {
	mode = "none";
	print "\\subsection*{Authorization}\n" $3 >> fname;
	next;
}

/^[/ ]\*!! PURPOSE$/ {
	mode = "copy";
	print "\\subsection*{Purpose}" >> fname;
	next;
}

/^[/ ]\*!! EXAMPLES$/ {
	mode = "copy";
	print "\\subsection*{Examples}" >> fname;
	next;
}

/^[/ ]\*!! OPERANDS$/ {
	mode = "opers";
	next;
}

/^[/ ]\*!! OPTIONS$/ {
	mode = "options";
	next;
}

/^[/ ]\*! / {
	# verbatim line
	if (mode == "copy" || mode == "syntax")
		print substr($0, 5) >> fname;
}

/^[/ ]\*!$/ {
	if (mode == "copy" || mode == "syntax")
		print substr($0, 5) >> fname;
}
'
}

rm -f cp-cmd-list.tex
mkdir -p tex/

for srcf in ../../sys/cp/cmd_*.c ; do
	echo "Inspecting $srcf..."
	write_docs $srcf tex
	#write_docs $srcf txt
	#write_docs $srcf html
done

sort < cp-cmd-list.tex > cp-cmd-list.tex.2
mv cp-cmd-list.tex.2 cp-cmd-list.tex
