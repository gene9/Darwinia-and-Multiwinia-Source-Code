#!/usr/bin/perl
use File::Basename;
use File::Spec::Functions;
use File::Copy;
use File::Compare;

if( @ARGV != 2 )
{
	die "usage: $0 from-dir to-dir";
}

$fromdir = $ARGV[0];
$todir = $ARGV[1];

$base = dirname($0);
@rarcmp = ("perl", "--", catfile($base, "rarcmp.pl"));

opendir FROMDIR, "$fromdir"
	or die "Failed to list directory $fromdir: $!";
	
foreach my $entry (readdir(FROMDIR))
{
	next if $entry =~ /^\./;
	if( $entry =~ /\.dat$/i )
	{
		my $frompath = catfile( $fromdir, $entry );
		my $topath = catfile( $todir, $entry );
			
		if( system( @rarcmp, $frompath, $topath ) == 0 )
		{
			if( compare( $frompath, $topath ) != 0 )
			{
				print "$frompath => $topath\n";
				copy( $frompath, $topath )
					or die "Failed to copy $frompath => $topath";
			}
			else
			{
				print "$frompath identical to $topath\n";
			}
		}	
	}	
}
	
closedir FROMDIR;
