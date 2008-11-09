#!/usr/bin/perl
use warnings;
use strict;

# USAGE: ./PowerGatingParser.pl > PowerGatingLib.h

# Hash for seen opcodes
my %opcodes;

# File to read opcodes from
my $infile = 'X86GenInstrNames.inc';
open my $fh, '<', $infile or die "Failed to open $infile: $!";

# Read the file
while (<$fh>) {
	/(\S+)\s*=\s*\d+/;
	my $a = $1;

	# Print out the opcodes if this is the first time we've seen them
	print "\tcase X86::$a:\n" if $a;
}
print "\tdefault:\n";
print "\t\treturn ((gatingmask_t)0 - 1);\n";
print "\t\tbreak;\n";

