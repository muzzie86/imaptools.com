#! /usr/bin/perl -w
    eval 'exec /usr/bin/perl -S $0 ${1+"$@"}'
        if 0; #$running_under_some_shell

use strict;
use File::Find ();
use File::Basename;

# Set the variable $File::Find::dont_use_nlink if you're using AFS,
# since AFS cheats.

# for the convenience of &wanted calls, including -eval statements:
use vars qw/*name *dir *prune/;
*name   = *File::Find::name;
*dir    = *File::Find::dir;
*prune  = *File::Find::prune;

sub wanted;
sub Usage;

my $path = shift @ARGV || Usage();
my $patt = shift @ARGV || Usage();
my $fld  = shift @ARGV || Usage();

my @files = ();

# Traverse desired filesystems
File::Find::find({wanted => \&wanted}, $path);

for my $x (@files) {
    my ($name, $path, $suf) = fileparse($x, '\.shp');
    if (! -r $x || ! -w "$path$name.dbf") {
        die "File not readable/writeable: $path$name.*\n";
    }
    print "./dbfarea -s -f AREA $x tmp-$$\n";
    print "mv tmp-$$.dbf $path$name.dbf\n"
}

exit;


sub wanted {
    /^$patt\.shp\z/s &&
    push @files, $name;
}

sub Usage {
    die "Usage: add-area.pl <path to data> <file pattern> <new field name>\n" .
        "       EX: ./add-area.pl /u/data/tiger2004fe water AREA\n";
}
