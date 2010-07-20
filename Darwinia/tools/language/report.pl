#!/usr/bin/perl -w
use strict;

use Getopt::Long;
use File::Temp qw(tempfile);
use Data::Dumper;

my $debugging;

# =======================================
# Print usage message
# =======================================
sub print_usage {
    print <<USAGE_EOF;
Usage:
  <scriptname> [--output <outputfile>] [--report <reportfile>]
  [--summary <summaryfile>] [--auto] <inputfile>
Options:
  -h, --help, -?  Display this usage message and exit.
  -d, --debug     Print script debugging messages.
  -o, --output    Write new strings into specified file (default: strings.txt).
  -r, --report    Write change report to specified file (default: report.txt).
  -s, --summary   Write summary into specified file (default: stdout).
  -n, --english   Write new English strings to specified file (default: none).
  -a, --auto      Output names depend on input name, where not defined.
  -q, --quiet     Don't print summary on stdout, as per default.
  -v, --verbose   Verbose summary of insertions, deletions and modifications.
  -k, --keep      Keep old translations for modified strings.
USAGE_EOF
}

# =======================================
# Configure - read command line, etc.
# =======================================
sub configure {
    my ($help, $auto_names, $quiet) = ('', '', '');
    $debugging = '';
    my %config = (
        'old_eng_fname'  => 'english.old.txt',
        'new_eng_fname'  => 'english.txt',
        'old_lang_fname' => '',
        'new_lang_fname' => '',
        'report_fname'   => '',
        'summary_fname'  =>  0,
        'eng_out_fname'  => '',
        'keep_old_trans' => '',
        'verbose'        => ''
    );
    
    GetOptions(
        'h|help|?'     => \$help,
        'd|debug'      => \$debugging,
        'oldeng=s'     => \$config{'old_eng_fname'},
        'neweng=s'     => \$config{'new_eng_fname'},
        'o|output=s'   => \$config{'new_lang_fname'},
        'r|report=s'   => \$config{'report_fname'},
        's|summary:s'  => \$config{'summary_fname'},
        'n|english=s'  => \$config{'eng_out_fname'},
        'a|auto'       => \$auto_names,
        'q|quiet'      => \$quiet,
        'k|keep'       => \$config{'keep_old_trans'},
        'v|verbose'    => \$config{'verbose'}
    ) or do { $help = 1; };
    
    if ( $help or $#ARGV != 0 ) {
        print_usage();
        exit();
    }
    
    if ( $#ARGV == 0 ) {
        $config{'old_lang_fname'} = $ARGV[0];
    }
    
    if ( $auto_names ) {
        my $base = $config{'old_lang_fname'};
        $base =~ s/\.txt$//;
        if ( $base eq $config{'old_lang_fname'} ) {
            die "Can't decide automatic output file names.\n";
        }
        
        $config{'new_lang_fname'} = $base.'-strings.txt' if ( $config{'new_lang_fname'} eq '' );
        $config{'report_fname'} = $base.'-report.txt' if ( $config{'report_fname'} eq '' );
        $config{'summary_fname'} = $base.'-summary.txt' if ( $config{'summary_fname'} eq '' );
    } else {
        $config{'new_lang_fname'} = 'strings.txt' if ( $config{'new_lang_fname'} eq '' );
        $config{'report_fname'} = 'report.txt' if ( $config{'report_fname'} eq '' );
        $config{'summary_fname'} = 'summary.txt' if ( $config{'summary_fname'} eq '' );
    }
    
    $config{'summary_fname'} = '' unless ( $config{'summary_fname'} or $quiet );
    
    print Data::Dumper->Dump( [ \%config ], [ 'config' ] ) if $debugging;
    return %config;
}

# =======================================
# Misc. utility functions
# =======================================
sub trim {
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}

sub apply_key_renamings {
    my @nums = @{shift(@_)}; my %changes = %{shift(@_)};
    my @newnums = ();
    
    foreach my $key ( @nums ) {
        if ( ($key =~ m/^[a-zA-Z0-9_]+$/) and defined( $changes{$key} ) ) {
            if ( ref( $changes{$key} ) ) {
                push( @newnums, @{$changes{$key}} );
            } else {
                push( @newnums, $changes{$key} );
            }
        } else {
            push( @newnums, $key );
        }
    }
    
    return @newnums;
}

{
    my $kfmt = '%-34s';

    sub make_line {
        return sprintf( $kfmt.'%s', @_ )."\n";
    }

    sub set_kfmt {
        ($kfmt) = @_;
    }
}

# =======================================
# Read strings file into memory
# =======================================
sub read_strings_file {
    my ( $fname ) = @_; my %lines = ();
    my %keys = (); my @nums = (); my $maxln = 0;
    open( FIN, "<$fname" ) or die "Can't open $fname for reading: $!\n";
    while ( <FIN> ) {
        chomp;
        if ( m/^\s*(\w+)\s*([^#]*)/ ) {
            if ( $debugging && defined( $keys{$1} ) ) {
                my $the_same = ($keys{$1} eq trim( $2 ));
                print "WARNING: Key ($1) defined twice in file ($fname).\n";
                print "   The latest two definitions ($1) are ".($the_same?"the same":"different").".\n";
            }
            $keys{$1} = trim( $2 );
            push( @nums, $1 );
            $maxln = length( $1 ) if length( $1 ) > $maxln;
        } else {
            push( @nums, $_ );
        }
    }
    close( FIN );
    %lines = (
        'file' => $fname,   # Name of the file
        'keys' => \%keys,   # map of ids -> strings
        'nums' => \@nums,   # keep ordering and comments
        'klen' => $maxln    # Keep maximum key length
    );
    return %lines;
}

# =======================================
# Replace known substitutions
# =======================================
sub substitute_substitutions {
    my %lines = %{shift(@_)}; my %keys = %{$lines{'keys'}};
    #print Data::Dumper->Dump( [ \%lines ], [ 'substitute_substitutions_lines' ] );
    
    while ( my ($key, $val) = each( %keys ) ) {
        $key =~ s/^control_(leftclick|rightclick|leftmousebutton|leftclicking)$/part_$1/;
        
        $val =~ s/\[ALT\]/[KEYTASKMANAGERDISPLAY]/g;
        $val =~ s/\[LEFTCLICK\]/[part_leftclick]/g;
        $val =~ s/\[LEFTCLICKING\]/[part_leftclicking]/g;
        $val =~ s/\[RIGHTCLICK\]/[part_rightclick]/g;
        $val =~ s/\[LEFTMOUSEBUTTON\]/[part_leftmousebutton]/g;
        
        $val =~ s/ h / [KEYSEPULVEDAHELP] /gi if ( $key !~ m/^bootloader/ );
        $val =~ s/([Hh]old) down/$1/gi;

        $val =~ s/\[KEY(UP|DOWN|LEFT|RIGHT)\]/[KEYMENU$1]/gi if ( $key =~ m/^(help_taskmanager_2|help_showobjectives_icons)/ );

        $val =~ s/\[KEY(UP|DOWN|LEFT|RIGHT|FORWARDS|BACKWARDS|ZOOM)\]/[KEYCAMERA$1]/g;
        $val =~ s/\[KEYCHATLOG(DISPLAY)?\]/[KEYGESTURESCHATLOG]/g;
        $val =~ s/\[KEYDESELECT\]/[KEYUNITDESELECT]/g;
        $val =~ s/\[KEYTASKMANAGERDISPLAY\]/[KEYGESTURESTASKMANAGERDISPLAY]/g;
        
        $keys{$key} = $val;
    }
    
    $lines{'keys'} = \%keys;
    return %lines;
}

# =======================================
# Expand parts to find actual text
# =======================================
sub expand_parts {
    my %lines = %{shift(@_)}; my %keys = %{$lines{'keys'}};
    #print Data::Dumper->Dump( [ \%lines ], [ 'expand_parts_lines' ] );
    my %changes = ();
    
    while ( my ($key, $val_kbd) = each(%keys) ) {
        my $val_xin = $val_kbd;
        
        my $done = '';
        until ( $done ) {
            if ( $val_kbd =~ m/\[(\s?)([a-z][a-zA-Z0-9_]*)\]/ ) {
                my ( $s, $k, $v ) = ( '', $2, '' ); $s = $1 if defined( $1 );
                if ( defined( $keys{$k.'_kbd'} ) ) { $v = $keys{$k.'_kbd'}; }
                elsif ( defined( $keys{$k} ) ) { $v = $keys{$k}; }
                if ( $v eq '' ) {
                    $val_kbd =~ s/\[(\s?)$k\]//g;
                } else {
                    $val_kbd =~ s/\[(\s?)$k\]/$1${v}/g;
                }
            } else { $done = 1; }
        }

        $done = '';
        until ( $done ) {
            if ( $val_xin =~ m/\[(\s?)([a-z][a-zA-Z0-9_]*)\]/ ) {
                my ( $s, $k, $v ) = ( '', $2, '' ); $s = $1 if defined( $1 );
                if ( defined( $keys{$k.'_xin'} ) ) { $v = $keys{$k.'_xin'}; }
                elsif ( defined( $keys{$k} ) ) { $v = $keys{$k}; }
                if ( $v eq '' ) {
                    $val_xin =~ s/\[(\s?)$k\]//g;
                } else {
                    $val_xin =~ s/\[(\s?)$k\]/$1${v}/g;
                }
            } else { $done = 1; }
        }

        delete $keys{$key};
        if ( $val_kbd eq $val_xin ) {
            $keys{$key} = $val_kbd;
        } else {
            if ( $key =~ m/(_kbd)$/ ) {
                $keys{$key} = $val_kbd;
            } elsif ( $key =~ m/(_xin)$/ ) {
                $keys{$key} = $val_xin;
            } else {
                $keys{$key.'_kbd'} = $val_kbd;
                $keys{$key.'_xin'} = $val_xin;
                $changes{$key} = [ $key.'_kbd', $key.'_xin' ];
            }
        }
    }
    
    my @nums = apply_key_renamings( $lines{'nums'}, \%changes );
    
    $lines{'keys'} = \%keys;
    $lines{'nums'} = \@nums;
    return %lines;
}

# =======================================
# Delete parts
# =======================================
sub delete_parts {
    my %lines = %{shift(@_)}; my %keys = %{$lines{'keys'}};
    my @nums = (); my $key;
    #print Data::Dumper->Dump( [ \%lines ], [ 'delete_parts_lines' ] );
    
    foreach $key ( keys( %keys ) ) {
        if ( $key =~ m/^part_/ ) {
            delete $keys{$key};
        }
    }
    
    foreach $key ( @{$lines{'nums'}} ) {
        if ( $key !~ m/^part_[a-zA-Z0-9_]+$/ ) {
            push( @nums, $key );
        }
    }
    
    $lines{'keys'} = \%keys;
    $lines{'nums'} = \@nums;
    return %lines;
}

# =======================================
# Find strings which have just been renamed
# =======================================
sub find_renamings {
    my %old = %{shift(@_)}; %old = %{$old{'keys'}};
    my %new = %{shift(@_)}; %new = %{$new{'keys'}};
    my %renamed = ();
    #print Data::Dumper->Dump( [ \%old, \%new ], [ 'find_renamings_old', 'find_renamings_new' ] );
    
    foreach my $key ( keys( %old ) ) {
        if ( defined( $new{$key.'_kbd'} ) ) {
            if ( $old{$key} eq $new{$key.'_kbd'} ) {
                $renamed{$key} = $key.'_kbd';
            }
        } elsif ( defined( $new{$key.'_xin'} ) ) {
            if ( $old{$key} eq $new{$key.'_xin'} ) {
                $renamed{$key} = $key.'_xin';
            }
        }
    }
    
    return %renamed;
}

# =======================================
# Rename keys according to given mapping
# =======================================
sub apply_renamings {
    my %ren = %{shift(@_)}; my %lines = %{shift(@_)}; my %keys = %{$lines{'keys'}};
    #print Data::Dumper->Dump( [ \%ren, \%lines ], [ 'apply_renamings_ren', 'apply_renamings_lines' ] );
    
    foreach my $key ( keys( %keys ) ) {
        if ( defined( $ren{$key} ) ) {
            if ( !defined($keys{$ren{$key}}) and ( $key ne $ren{$key} ) ) {
                $keys{$ren{$key}} = $keys{$key};
                delete $keys{$key};
            } else {
                print "WARNING: Key '$ren{$key}' already exists. Bad rename.\n" if $debugging;
            }
        }
    }
    
    my @nums = apply_key_renamings( $lines{'nums'}, \%ren );
    
    $lines{'keys'} = \%keys;
    $lines{'nums'} = \@nums;
    return %lines;
}

# =======================================
# Identify changed entries
# =======================================
sub find_changes {
    my %old = %{shift(@_)}; %old = %{$old{'keys'}};
    my %new = %{shift(@_)}; %new = %{$new{'keys'}};
    my %changes = (); my ($key, $val, $op);
    #print Data::Dumper->Dump( [ \%old, \%new ], [ 'old', 'new' ] );
    
    while ( ($key, $val) = each( %old ) ) {
        if ( !defined( $new{$key} ) ) {
            $changes{$key} = 'deleted';
        } elsif ( $new{$key} ne $val ) {
            $changes{$key} = 'changed';
        }
    }
    
    while ( ($key, $val) = each( %new ) ) {
        if ( !defined( $old{$key} ) ) {
            $changes{$key} = 'inserted';
        }
    }
    
    # A few refinements to simplify things
    while ( ($key, $op) = each( %changes ) ) {
        if ( $op eq 'deleted' ) {
            if ( defined( $changes{$key.'_kbd'} ) and ( $changes{$key.'_kbd'} eq 'inserted' ) ) {
                if ( defined( $changes{$key.'_xin'} ) and ( $changes{$key.'_xin'} eq 'inserted' ) ) {
                    $changes{$key} = 'split';
                    $changes{$key.'_xin'} = 'dst';
                } else {
                    $changes{$key} = 'kbd';
                }
                $changes{$key.'_kbd'} = 'dst';
            } elsif ( defined( $changes{$key.'_xin'} ) and ( $changes{$key.'_xin'} eq 'inserted' ) ) {
                $changes{$key} = 'xin';
                $changes{$key.'_xin'} = 'dst';
            }
        } elsif ( $op eq 'inserted' ) {
            if ( $key =~ m/(.*)_xin$/ ) {
                if ( !defined( $changes{$1} ) and !defined( $changes{$1.'_kbd'} ) ) {
                    if ( defined( $old{$1.'_kbd'} ) and !defined( $old{$1} ) ) {
                        $changes{$key} = 'dst_from_kbd';
                    }
                }
            }
        }
    }
    
    return %changes;
}


# =======================================
# Make new strings file (uncommented)
# =======================================
sub generate_strings_file {
    my $fname = shift(@_); my %lines = %{shift(@_)};
    my %keys = %{$lines{'keys'}};
    
    return if ( !defined( $fname ) or ($fname eq '') );
    
    set_kfmt( '%-'.($lines{'klen'}+2).'s' );
    
    open( FOUT, ">$fname" ) or die "Can't open $fname for writing: $!\n";
    foreach my $key ( @{$lines{'nums'}} ) {
        if ( $key =~ m/^[a-zA-Z0-9_]+$/ ) {
            if ( defined( $keys{$key} ) ) {
                print FOUT make_line( $key, $keys{$key} );
            } else {
                print FOUT make_line( $key, '' );
                print "WARNING: No English translation for $key\n" if $debugging;
            }
        } else {
            print FOUT $key."\n";
        }
    }
    close( FOUT );
}

# =======================================
# REPORT
# =======================================
sub generate_summary {
    my $fname = shift(@_); my %changes = %{shift(@_)}; my %ren = %{shift(@_)};
    my $verbose = shift(@_);
    my %count = (); my ($key, $op); my @ops = ( 'auto-renamed', 'deleted', 'inserted', 'split' );
    my $file; my $file_open = 0;
    #Data::Dumper->Dump( [ \$fname, \%changes ], [ 'fname', 'changes' ] );
    
    return if ( defined( $fname ) and !$fname and ($fname ne '') );
    
    foreach $op ( @ops ) {
        $count{$op} = 0;
    }
    
    while ( ($key, $op) = each( %changes ) ) {
        if ( defined( $count{$op} ) ) {
            $count{$op}++;
        } else {
            $count{$op} = 1;
        }
    }
    if ( defined( $count{'dst_from_kbd'} ) ) {
        $count{'inserted'} += $count{'dst_from_kbd'};
    }
    $count{'auto-renamed'} = scalar keys( %ren );
    
    if ( defined( $fname ) and ($fname ne '') ) {
        open( $file, ">$fname" ) or die "Can't open $fname for writing: $!\n";
        $file_open = 1;
    } else {
        $file = \*STDOUT;
    }

    if ( $verbose ) {
        while ( ($key, $op) = each( %changes ) ) {
            printf( $file "Entry %s was %s.\n", $key, $op );
        }
        print $file "\n";
    }

    foreach $op ( @ops ) {
        printf( $file "%4d entries have been %s.\n", $count{$op}, $op );
    }
    
    close( $file ) if ( $file_open );
}

# =======================================
# REPORT
# =======================================
sub make_report_header {
    my ($ftrans) = @_;
    return <<END_RPT_HEAD
This report lists changes in $ftrans for the attention of the translator.

You may delete the items listed below as you correct them, to keep track.
=========================

END_RPT_HEAD
}

sub make_ins_warning {
    my ($lnum, $key, $ne) = @_;
    $lnum = sprintf( "%2d", $lnum );
    return <<END_INSWARN;
======== LINE $lnum ========
NEW     $key
English is:
  $ne

END_INSWARN
}

sub make_mod_warning {
    my ($lnum, $key, $oe, $ne, $ol) = @_;
    $lnum = sprintf( "%2d", $lnum );
    my $ans = <<END_MODWARN;
======== LINE $lnum ========
MODIFIED $key
English changed from:
  $oe
To:
  $ne
END_MODWARN
    if ( defined( $ol ) ) {
        $ans .= <<END_MODWARN2
Old translation was:
  $ol

END_MODWARN2
    }
    return $ans;
}

sub generate_report {
    my $ftrans = shift(@_); my $freport = shift(@_); my %changes = %{shift(@_)};
    my %oe = %{shift(@_)}; my %ne = %{shift(@_)}; my %ol = %{shift(@_)};
    my $keep_old_trans = shift(@_);
    my %oe_keys = %{$oe{'keys'}}; my %ne_keys = %{$ne{'keys'}}; my %ol_keys = %{$ol{'keys'}};
    my $linenum = 1;
    #Data::Dumper->Dump( [ \$fname, \%changes ], [ 'fname', 'changes' ] );
    
    my ( $blanklines, $blank ) = ( 0, 0 );
    set_kfmt( '%-'.($ne{'klen'}+2).'s' );
    
    open( FOUT, ">$ftrans" ) or die "Can't open $ftrans for writing: $!\n";
    open( FRPT, ">$freport" ) or die "Can't open $freport for writing: $!\n";
    
    print FRPT make_report_header( $ftrans );
    
    foreach my $key ( @{$ne{'nums'}} ) {
        $blank = 0;
        if ( $key =~ m/^[a-zA-Z0-9_]+$/ ) {
            if ( defined( my $op = $changes{$key} ) ) {
                if ( ($op eq 'deleted') or ($op eq 'split') or ($op eq 'xin') or ($op eq 'kbd') ) { # Skip this one
                    $blank = 1;
                } elsif ( $op eq 'inserted' ) {
                    print FRPT make_ins_warning( $linenum, $key, $ne_keys{$key} );
                    if ( defined( $ol_keys{$key} ) ) {
                        print FOUT make_line( $key, $ol_keys{$key} );
                    } else {
                        print FOUT make_line( $key, '' );
                    }
                } elsif ( $op eq 'changed' ) {
                    print FRPT make_mod_warning( $linenum, $key, $oe_keys{$key}, $ne_keys{$key}, $ol_keys{$key} );
                    if ( defined( $ol_keys{$key} ) && $keep_old_trans ) {
                        print FOUT make_line( $key, $ol_keys{$key} );
                    } else {
                        print FOUT make_line( $key, '' );
                    }
                } elsif ( $op eq 'dst' ) {
                    my $old_key = $key; $old_key =~ s/_(xin|kbd)$//;
                    print FRPT make_mod_warning( $linenum, $key, $oe_keys{$old_key}, $ne_keys{$key}, $ol_keys{$old_key} );
                    print FOUT make_line( $key, '' );
                } elsif ( $op eq 'dst_from_kbd' ) {
                    my $old_key = $key; $old_key =~ s/_xin$/_kbd/;
                    print FRPT make_mod_warning( $linenum, $key, $oe_keys{$old_key}, $ne_keys{$key}, $ol_keys{$old_key} );
                    print FOUT make_line( $key, '' );
                } else {
                    print "UNKNOWN OPERATION: $op\n" if $debugging;
                    $blank = 1;
                }
            } else {
                if ( defined( $ol_keys{$key} ) ) {
                    print FOUT make_line( $key, $ol_keys{$key} );
                } else {
                    print FRPT make_ins_warning( $linenum, $key, $ne_keys{$key} );
                    print FOUT make_line( $key, '' );
                }
            }
        } else {
            $key =~ s/\s+$//;
            $blank = 1;
            if ( $key eq '' ) {
                if ( $blanklines++ < 3 ) {
                    print FOUT "\n";
                    $linenum++;
                }
            } elsif ( $key !~ m/^#- / ) { # Ignore lines starting "#- "
                print FOUT $key."\n";
                $blank = 0;
            }
        }
        
        unless ( $blank ) {
            $blanklines = 0;
            $linenum++;
        }
    }
    close( FRPT );
    close( FOUT );
}

# =======================================
# MAIN PROGRAM
# =======================================

# ---------------------------------------
# Configure the options
my %config = configure();

# ---------------------------------------
# Read the input files

# Old English, New English, Old Translation (Language)
my (%oe, %ne, %ol);

%oe = read_strings_file( $config{'old_eng_fname'} );
%ne = read_strings_file( $config{'new_eng_fname'} );
%ol = read_strings_file( $config{'old_lang_fname'} );

# ---------------------------------------
# Apply known substitutions

%oe = substitute_substitutions( \%oe );
%ne = substitute_substitutions( \%ne );
%ol = substitute_substitutions( \%ol );

# ---------------------------------------
# Run expansions

%oe = expand_parts( \%oe );
%ne = expand_parts( \%ne );
%ol = expand_parts( \%ol );

# ---------------------------------------
# Delete parts, they're all spent

%oe = delete_parts( \%oe );
%ne = delete_parts( \%ne );
%ol = delete_parts( \%ol );

# ---------------------------------------
# Rename keys

my %renamed = find_renamings( \%oe, \%ne );

%oe = apply_renamings( \%renamed, \%oe );
%ol = apply_renamings( \%renamed, \%ol );

# ---------------------------------------
# Detect changes, additions and deletions

my %changes = find_changes( \%oe, \%ne );

# ---------------------------------------
# Write English language file

generate_strings_file( $config{'eng_out_fname'}, \%ne );

# ---------------------------------------
# Write report (commented language file)

generate_report( $config{'new_lang_fname'}, $config{'report_fname'}, \%changes, \%oe, \%ne, \%ol, $config{'keep_old_trans'} );

# ---------------------------------------
# Write a summary

generate_summary( $config{'summary_fname'}, \%changes, \%renamed, $config{'verbose'} );
