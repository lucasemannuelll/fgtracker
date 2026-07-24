#ifndef DB_H
#define DB_H

#include "models.h"
#include <sqlite3.h>

int db_open(sqlite3 **db, const char *path);
void db_close(sqlite3 *db);

int db_insert_session(sqlite3 *db, int fgm, int fga);
int db_update_session(sqlite3 *db, int id, int fgm, int fga);
int db_delete_session(sqlite3 *db, int id);

int db_get_session(sqlite3 *db, int id, Session *out);
int db_get_all_sessions(sqlite3 *db, Session *out, int capacity);
int db_get_sessions_filtered(sqlite3 *db, Session *out, int capacity,
                            const char *time_filter);

#endif
