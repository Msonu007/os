# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(change-cpu) begin
(change-cpu) PASS
(change-cpu) end
EOF
pass;