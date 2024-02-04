# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-run-batch) begin
(cfs-run-batch) Test your scheduler running batch processes.
(cfs-run-batch) To pass, this needs to run to completion without error.
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) fib of 30 is 832040
(cfs-run-batch) PASS
(cfs-run-batch) end
EOF
pass;
