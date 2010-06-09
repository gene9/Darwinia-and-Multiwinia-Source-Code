#!/usr/bin/perl 
die "usage: $0 file new-origin" if (@ARGV < 2);

my $filename = $ARGV[0];
my $from = "MWORIGINMWORIGINMWORIGIN";
my $to = $ARGV[1];

my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size, $atime,$mtime,$ctime,$blksize,$blocks) = stat($filename);

open I, "<:raw", $filename or die "Failed to open $filename $! for reading";
binmode I;
sysread( I, $data, $size );
close I;

$to = substr($to, 0, length($from));
while( length($to) < length($from) )
{
	$to .= chr(0);
}

$data =~ s/$from/$to/og;

open O, ">:raw", $filename or die "Failed to open $filename $! for writing";
binmode O;
syswrite( O, $data );
close O;
