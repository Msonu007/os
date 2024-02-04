# -*- perl -*-
use tests::tests;
use tests::threads::alarm;
check_alarm (7);
check_system_idle ();
pass;
