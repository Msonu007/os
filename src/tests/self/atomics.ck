# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(atomics) begin
(atomics) PASS
(atomics) end
EOF
pass;