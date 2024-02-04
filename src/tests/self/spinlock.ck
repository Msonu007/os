# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(spinlock) begin
(spinlock) PASS
(spinlock) end
EOF
pass;