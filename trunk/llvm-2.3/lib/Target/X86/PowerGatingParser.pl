#!/usr/bin/perl
use warnings;
use strict;

# USAGE: ./PowerGatingParser.pl > PowerGatingLib.h

# Hash for seen opcodes
my %opcodes;

# File to read opcodes from
my $infile = 'PowerGating.h';
open my $fh, '<', $infile or die "Failed to open $infile: $!";

# Read the file
while (<$fh>) {

	# Get the opcodes
	my ($a, $b, @extra) = split /,/, $_;
	$a =~ /(X86::\w+)/;
	$a = $1;
	$b =~ /(X86::\w+)/;
	$b = $1;

	# Increment the opcode counts
	$opcodes{$a}++;
	$opcodes{$b}++;

	# Print out the opcodes if this is the first time we've seen them
	print "\tcase $a:\n" if $a and $opcodes{$a} == 1;
	print "\tcase $b:\n" if $b and $opcodes{$b} == 1;
}
print "\tdefault:\n";
print "\t\treturn ((gatingmask_t)0 - 1);\n";
print "\t\tbreak;\n";

