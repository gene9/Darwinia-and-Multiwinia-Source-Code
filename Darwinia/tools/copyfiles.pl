#!/usr/bin/perl
# Copy files listed on standard input from source dir to dest dir

use File::Copy;
use File::Basename;

$sourcedir = $ARGV[0];
$destdir = $ARGV[1];
if (! -d $sourcedir || ! -d $destdir) {
	print STDERR "usage: $0 sourcedir destdir\n";
	exit 1;
}

while(<STDIN>) {
	chomp;
	my $dest = "$destdir/$_";
	my $source = "$sourcedir/$_";

	if (! -f $source) {
		die "$0: $source $!";
	}

	mkdir(dirname($dest));
	
	print "$source => $dest\n";
	copy("$sourcedir/$_", $dest)
		or die "Copy failed: $!";	
}
