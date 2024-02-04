# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-sleeper-minvruntime) begin
(cfs-sleeper-minvruntime) PASS
(cfs-sleeper-minvruntime) end
EOF
pass;
