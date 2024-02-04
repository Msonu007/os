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
(balance-synch2) begin
(balance-synch2) This test is very unforgiving of race conditions.
(balance-synch2) It will not run fast!.
(balance-synch2) Running 1000 tests.
(balance-synch2) Finished test 0
(balance-synch2) Finished test 10
(balance-synch2) Finished test 20
(balance-synch2) Finished test 30
(balance-synch2) Finished test 40
(balance-synch2) Finished test 50
(balance-synch2) Finished test 60
(balance-synch2) Finished test 70
(balance-synch2) Finished test 80
(balance-synch2) Finished test 90
(balance-synch2) Finished test 100
(balance-synch2) Finished test 110
(balance-synch2) Finished test 120
(balance-synch2) Finished test 130
(balance-synch2) Finished test 140
(balance-synch2) Finished test 150
(balance-synch2) Finished test 160
(balance-synch2) Finished test 170
(balance-synch2) Finished test 180
(balance-synch2) Finished test 190
(balance-synch2) Finished test 200
(balance-synch2) Finished test 210
(balance-synch2) Finished test 220
(balance-synch2) Finished test 230
(balance-synch2) Finished test 240
(balance-synch2) Finished test 250
(balance-synch2) Finished test 260
(balance-synch2) Finished test 270
(balance-synch2) Finished test 280
(balance-synch2) Finished test 290
(balance-synch2) Finished test 300
(balance-synch2) Finished test 310
(balance-synch2) Finished test 320
(balance-synch2) Finished test 330
(balance-synch2) Finished test 340
(balance-synch2) Finished test 350
(balance-synch2) Finished test 360
(balance-synch2) Finished test 370
(balance-synch2) Finished test 380
(balance-synch2) Finished test 390
(balance-synch2) Finished test 400
(balance-synch2) Finished test 410
(balance-synch2) Finished test 420
(balance-synch2) Finished test 430
(balance-synch2) Finished test 440
(balance-synch2) Finished test 450
(balance-synch2) Finished test 460
(balance-synch2) Finished test 470
(balance-synch2) Finished test 480
(balance-synch2) Finished test 490
(balance-synch2) Finished test 500
(balance-synch2) Finished test 510
(balance-synch2) Finished test 520
(balance-synch2) Finished test 530
(balance-synch2) Finished test 540
(balance-synch2) Finished test 550
(balance-synch2) Finished test 560
(balance-synch2) Finished test 570
(balance-synch2) Finished test 580
(balance-synch2) Finished test 590
(balance-synch2) Finished test 600
(balance-synch2) Finished test 610
(balance-synch2) Finished test 620
(balance-synch2) Finished test 630
(balance-synch2) Finished test 640
(balance-synch2) Finished test 650
(balance-synch2) Finished test 660
(balance-synch2) Finished test 670
(balance-synch2) Finished test 680
(balance-synch2) Finished test 690
(balance-synch2) Finished test 700
(balance-synch2) Finished test 710
(balance-synch2) Finished test 720
(balance-synch2) Finished test 730
(balance-synch2) Finished test 740
(balance-synch2) Finished test 750
(balance-synch2) Finished test 760
(balance-synch2) Finished test 770
(balance-synch2) Finished test 780
(balance-synch2) Finished test 790
(balance-synch2) Finished test 800
(balance-synch2) Finished test 810
(balance-synch2) Finished test 820
(balance-synch2) Finished test 830
(balance-synch2) Finished test 840
(balance-synch2) Finished test 850
(balance-synch2) Finished test 860
(balance-synch2) Finished test 870
(balance-synch2) Finished test 880
(balance-synch2) Finished test 890
(balance-synch2) Finished test 900
(balance-synch2) Finished test 910
(balance-synch2) Finished test 920
(balance-synch2) Finished test 930
(balance-synch2) Finished test 940
(balance-synch2) Finished test 950
(balance-synch2) Finished test 960
(balance-synch2) Finished test 970
(balance-synch2) Finished test 980
(balance-synch2) Finished test 990
(balance-synch2) PASS
(balance-synch2) end
EOF
pass;
