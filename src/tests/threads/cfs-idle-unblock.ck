# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(cfs-idle-unblock) begin
(cfs-idle-unblock) PASS
(cfs-idle-unblock) end
EOF
pass;
