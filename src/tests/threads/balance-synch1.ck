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
(balance-synch1) begin
(balance-synch1) Load balancing test is run multiple times.
(balance-synch1) to look for race conditions that may occur during.
(balance-synch1) load balancing..
(balance-synch1) This test will not run fast!.
(balance-synch1) Running 500 tests.
(balance-synch1) Finished test 0
(balance-synch1) Finished test 10
(balance-synch1) Finished test 20
(balance-synch1) Finished test 30
(balance-synch1) Finished test 40
(balance-synch1) Finished test 50
(balance-synch1) Finished test 60
(balance-synch1) Finished test 70
(balance-synch1) Finished test 80
(balance-synch1) Finished test 90
(balance-synch1) Finished test 100
(balance-synch1) Finished test 110
(balance-synch1) Finished test 120
(balance-synch1) Finished test 130
(balance-synch1) Finished test 140
(balance-synch1) Finished test 150
(balance-synch1) Finished test 160
(balance-synch1) Finished test 170
(balance-synch1) Finished test 180
(balance-synch1) Finished test 190
(balance-synch1) Finished test 200
(balance-synch1) Finished test 210
(balance-synch1) Finished test 220
(balance-synch1) Finished test 230
(balance-synch1) Finished test 240
(balance-synch1) Finished test 250
(balance-synch1) Finished test 260
(balance-synch1) Finished test 270
(balance-synch1) Finished test 280
(balance-synch1) Finished test 290
(balance-synch1) Finished test 300
(balance-synch1) Finished test 310
(balance-synch1) Finished test 320
(balance-synch1) Finished test 330
(balance-synch1) Finished test 340
(balance-synch1) Finished test 350
(balance-synch1) Finished test 360
(balance-synch1) Finished test 370
(balance-synch1) Finished test 380
(balance-synch1) Finished test 390
(balance-synch1) Finished test 400
(balance-synch1) Finished test 410
(balance-synch1) Finished test 420
(balance-synch1) Finished test 430
(balance-synch1) Finished test 440
(balance-synch1) Finished test 450
(balance-synch1) Finished test 460
(balance-synch1) Finished test 470
(balance-synch1) Finished test 480
(balance-synch1) Finished test 490
(balance-synch1) PASS
(balance-synch1) end
EOF
pass;
