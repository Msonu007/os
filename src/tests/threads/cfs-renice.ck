# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-renice) begin
(cfs-renice) PASS
(cfs-renice) end
EOF
pass;
