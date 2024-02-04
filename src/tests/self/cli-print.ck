# -*- perl -*-
use strict;
use warnings;
use tests::tests;
our ($test);
my (@output) = read_text_file("$test.output");

common_checks ("run", @output);
my $count = 0;
foreach (@output) {
	my ($a, $b, $c) = /(.*) Printing Line (.*)/ or next;
	$count = $count + 1;
}

if ($count != 20) {
	fail;
}

pass;