# Global variable
@addresses = ();

# Main starts here
if (@ARGV != 2) {
    printf "Arguments: $0 stack-trace map-file\n";
    exit 1;
}

($stackfile, $mapfile) = @ARGV;

print "Reading MAP file\n";
parseMap($mapfile);
parseStack($stackfile);
exit 0;

sub parseMap {
    my $mapfile = shift;
    my $symbols = 0;

    open INPUT, "<$mapfile"
      or die "Failed to open $mapfile for reading: $1";
    while (<INPUT>) {
	if (/\S+\s+(\S+)\s+(\S+) ...\s*(\S+)/) {
	    my $sym = $1;
	    my $addr = $2;
	    my $place = $3;
	    push @addresses, [$sym, hex $addr, $place];
	    $symbols++;
	}
	elsif (/^([0-9a-f]+) (.) (\S+)/) {
	    my $sym = $3;
	    my $addr = $1;
	    my $place = $2;
	    push @addresses, [$sym, hex $addr, $place];
	    $symbols++;
	}
    }
    close INPUT;

    @addresses = sort { $$a[1] <=> $$b[1] } @addresses;
    print "$symbols symbols read\n";
}

sub parseStack {
    my $tracefile = shift;
    open INPUT, "<$tracefile"
      or die "Failed to open $tracefile for reading: $1";

    my $printing = 0;

    while (<INPUT>) {
	if (/retAddress = (\S+)/) {
	    if (!$printing) {
		print "Stack trace\n";
		$printing = 1;
	    }

	    resolveAddr(hex $1);
	}
	else {
	    if ($printing) {
		print "\n";
		$printing = 0;
	    }
	}
    }

    close INPUT;
}


sub resolveAddr {
    my $target = shift;
    my $bestaddr = "[Unknown]";
    my $bestsym = "[Unknown]";
    my $bestplace = "[Unknown]";

    foreach $x (@addresses) {
	my ($sym, $addr, $place) = @$x;

	last if ($addr > $target);

	$bestsym = $sym;
	$bestplace = $place;
	$bestaddr = $addr;
    }

    printf "%8x  %8x  $bestsym  ($bestplace)\n", $target, $bestaddr;
};
