# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::threads::alarm;
check_expected ([<<'EOF']);
(alarm-synch) begin
(alarm-synch) PASS
(alarm-synch) end
EOF
check_system_idle ();
pass;
