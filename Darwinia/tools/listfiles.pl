use File::Find;
use File::Glob ':glob';
use strict;

if (@ARGV < 1) {
	print STDERR "usage: directory [exclude-file]\n";
	exit 1;
}

my $excludefile = $ARGV[1];

if (defined($excludefile)) {
    open EXCLUDE, "<$excludefile"
	or die "Failed to open exclude file $excludefile for reading: $!";
}

my $directory = $ARGV[0];

chdir($directory)
    or die "Failed to change to directory $directory: $!";

my @excludes = ();
my @shortexcludes = ();
if (defined($excludefile)) {
    @excludes = ();
	while (<EXCLUDE>) {
		s|\r\n|\n|g;
		chomp;
		s|\\|/|g;
		if (/^\*\*\/(.*)$/) {
			push @shortexcludes, $1;
		}		
		else {
			push @excludes, bsd_glob($_);
		}
    }
    close EXCLUDE;
}

sub wanted {
    my $name = $File::Find::name;
    $name =~ s%^\./%%;

	my $shortre = quotemeta $_;
	if (grep /^${shortre}$/, @shortexcludes) {
		$File::Find::prune = 1;
		return;
    }

    my $re = quotemeta $name;
    my @results = grep /^${re}$/i, @excludes;
    if (@results) {
		$File::Find::prune = 1;
		return;
    }
	
	if (-f $_) {
		print "$name\n";
	}
}

find(\&wanted, ".");
