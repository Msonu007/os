# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(fd-management) begin
(fd-management) create quux00.dat
(fd-management) create quux01.dat
(fd-management) create quux02.dat
(fd-management) create quux03.dat
(fd-management) create quux04.dat
(fd-management) create quux05.dat
(fd-management) open quux00.dat
(fd-management) write quux00.dat
(fd-management) open quux01.dat
(fd-management) write quux01.dat
(fd-management) open quux02.dat
(fd-management) write quux02.dat
(fd-management) open quux03.dat
(fd-management) write quux03.dat
(fd-management) open quux04.dat
(fd-management) write quux04.dat
(fd-management) open quux05.dat
(fd-management) write quux05.dat
(fd-management) close quux00.dat
(fd-management) close quux02.dat
(fd-management) close quux04.dat
(fd-management) create quux06.dat
(fd-management) open quux06.dat
(fd-management) write quux06.dat
(fd-management) create quux07.dat
(fd-management) open quux07.dat
(fd-management) write quux07.dat
(fd-management) create quux08.dat
(fd-management) open quux08.dat
(fd-management) write quux08.dat
(fd-management) seek quux01.dat
(fd-management) memcmp quux01.dat
(fd-management) seek quux03.dat
(fd-management) memcmp quux03.dat
(fd-management) seek quux05.dat
(fd-management) memcmp quux05.dat
(fd-management) starting 3000 open/close
(fd-management) open/close completed
(fd-management) end
fd-management: exit(0)
EOF
pass;
