# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-short-sleeper) begin
(cfs-short-sleeper) PASS
(cfs-short-sleeper) end
EOF
pass;
