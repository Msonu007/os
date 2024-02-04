# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(all-list) begin
Thread initial has TID 1, on CPU 0
Thread idle_cpu0 has TID 2, on CPU 0
Thread idle_cpu1 has TID 4, on CPU 1
Thread t1 has TID 5, on CPU 1
Thread t2 has TID 6, on CPU 0
Thread t3 has TID 7, on CPU 1
Thread t4 has TID 8, on CPU 0
==================================
Thread initial has TID 1, on CPU 0
Thread idle_cpu0 has TID 2, on CPU 0
Thread idle_cpu1 has TID 4, on CPU 1
(all-list) PASS
(all-list) end
EOF
pass;