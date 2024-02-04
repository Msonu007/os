# -*- perl -*-
use tests::tests;
use tests::threads::alarm;
check_alarm (1);
check_system_idle ();
pass;