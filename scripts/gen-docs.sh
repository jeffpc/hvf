write_docs()
{
cat "$1" | awk -v fmt="$2" '
BEGIN{
	docdir = "Documentation/commands";

	if (fmt == "txt") {
		cmd_title_open = "";
		cmd_title_close = "";
		sec_hdr_open = "";
		sec_hdr_close = "";
		par_open = "";
		par_close = "";
		pre_open = "";
		pre_close = "";
	} else if (fmt == "html") {
		cmd_title_open = "<h1>";
		cmd_title_close = "</h1>";
		sec_hdr_open = "<h2>";
		sec_hdr_close = "</h2>";
		par_open = "<p>";
		par_close = "</p>";
		pre_open = "<pre>";
		pre_close = "</pre>";
	} else {
		printf "ERROR: unknown format \"%s\"\n", fmt;
		exit;
	}

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
		fname = fname $i "_";
	}
	name = substr(name, 1, length(name)-1);
	fname = docdir "/" fmt "/" substr(fname, 1, length(fname)-1) "." fmt;

	print "Writing out ", fname;

	print cmd_title_open > fname;
	print name >> fname;
	print cmd_title_close >> fname;
}

/^[/ ]\*!! AUTH / {
	# output an authorization block
	print sec_hdr_open >> fname;
	print "Authorization" >> fname;
	print sec_hdr_close par_open >> fname;
	print $3 >> fname;
	printf par_close >> fname;
}

/^[/ ]\*!! PURPOSE$/ {
	# output purpose section header
	print sec_hdr_open >> fname;
	print "Purpose" >> fname;
	print sec_hdr_close >> fname;
}

/^[/ ]\*!! NOTES$/ {
	# output notes section header
	print sec_hdr_open >> fname;
	print "Usage Notes" >> fname;
	print sec_hdr_close >> fname;
}

/^[/ ]\*!p / {
	# preformated verbatim line
	print pre_open substr($0, 6) pre_close >> fname;
}

/^[/ ]\*! / {
	# verbatim line
	print substr($0, 5) >> fname;
}

/^[/ ]\*!$/ {
	# just make a new paragraph
	print substr($0, 5) >> fname;
}
'
}

mkdir -p Documentation/commands/txt
mkdir -p Documentation/commands/html

for srcf in cp/cmd_*.c ; do
	echo "Inspecting $srcf..."
	write_docs $srcf txt
	write_docs $srcf html
done
