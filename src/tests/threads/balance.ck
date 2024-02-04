# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::threads::balance;

our ($test);
my (@output) = read_text_file("$test.output");

common_checks ("run", @output);

my (@kernel_ticks) = ();
my (@idle_ticks) = ();
foreach (@output) {
	my ($a, $b, $c) = /CPU(\d+): (\d+) idle ticks, (\d+) kernel ticks(.*)/ or next;
	push (@idle_ticks, $b);
	push (@kernel_ticks, $c);
}

load_balance_check (@kernel_ticks);
idle_check (\@idle_ticks, \@kernel_ticks);

check_expected ([<<'EOF']);
(balance) begin
(balance) This test creates short-running threads on one CPU.
(balance) and long-running threads on the other..
(balance) Checks that one CPU finishes quickly and attempts.
(balance) to pull tasks from the other.
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) fib of 35 is 9227465
(balance) PASS
(balance) end
EOF
pass;
