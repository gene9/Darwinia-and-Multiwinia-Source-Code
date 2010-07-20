while (<>) {
	exit 0 if (/demo_endoffile/);
	print;
}
print STDERR "WARNING: no demo_endoffile tag found\n";
exit 1;

