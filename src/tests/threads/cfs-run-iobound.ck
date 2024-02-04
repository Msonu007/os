# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-run-iobound) begin
(cfs-run-iobound) Test your scheduler running many 'sleeper' threads by contending.
(cfs-run-iobound) for a lock. To pass, this needs to run to completion without error.
(cfs-run-iobound) Running test 10 times...
(cfs-run-iobound) Finished test 0
(cfs-run-iobound) Finished test 1
(cfs-run-iobound) Finished test 2
(cfs-run-iobound) Finished test 3
(cfs-run-iobound) Finished test 4
(cfs-run-iobound) Finished test 5
(cfs-run-iobound) Finished test 6
(cfs-run-iobound) Finished test 7
(cfs-run-iobound) Finished test 8
(cfs-run-iobound) Finished test 9
(cfs-run-iobound) PASS
(cfs-run-iobound) end
EOF
pass;
