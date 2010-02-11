#ifndef __Test_h
#define __Test_h

#ifndef FORTE_NO_MYSQL
void setup_mysql();
#endif

#ifndef FORTE_NO_POSTGRESQL
void setup_pgsql();
#endif

#ifndef FORTE_NO_SQLITE
void setup_sqlite();
#endif

void autoconnection_test();
void run_tests();

#endif
