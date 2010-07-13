#!/usr/bin/perl 
die "usage: $0 file from-re to" if (@ARGV < 3);

my $filename = $ARGV[0];
my $from = $ARGV[1];
my $to = $ARGV[2];

my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size, $atime,$mtime,$ctime,$blksize,$blocks) = stat($filename);

open I, "<:raw", $filename or die "Failed to open $filename $! for reading";
binmode I;
sysread( I, $data, $size );
close I;

$data =~ s/$from/$to/og;

open O, ">:raw", $filename or die "Failed to open $filename $! for writing";
binmode O;
syswrite( O, $data );
close O;
