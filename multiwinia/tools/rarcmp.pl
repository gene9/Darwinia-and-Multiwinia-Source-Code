#!/usr/bin/perl
use File::Basename;
use File::Spec::Functions;

die "usage: $0 rarfile1 rarfile2" if( @ARGV != 2 );
die "Failed to find $ARGV[0]" if( ! -f "$ARGV[0]" );
die "Failed to find $ARGV[1]" if( ! -f "$ARGV[1]" );

$base = dirname($0);
$rar = catfile($base, "rar");

sub descriptor
{
	my $filename = shift @_;
	
	open A, "\"$rar\" v \"$filename\"|"
		or die "Failed to open rar v \"$filename\" for reading: $!";
		
	my $state = 0;
	my @details = ();
	
	while( <A> )
	{
		if( $state == 0 )
		{
			$state = 1 if( /^----/ );
		}
		elsif( $state == 1 )
		{
			last if ( /^----/ );
				
			chomp;

			my $filename = $_;
			my $details = <A>;
			
			$filename =~ s/^.//;
			my @parts = split /\s+/, $details;
			my $size = $parts[1];
			my $crc = $parts[7];
						
			push @details, "$filename $size $crc";
		}
	}	
	close A;
	
	my $descriptor = join("\n", sort @details);
	return $descriptor;
}
	
if( descriptor($ARGV[0]) eq descriptor($ARGV[1]) )
{
	exit 0;
}
else
{
	exit 1;
}
	
