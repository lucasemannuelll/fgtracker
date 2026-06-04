#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include "models.h"

int db_open(sqlite3 **db, const char *path);

void db_close(sqlite3 *db);

int db_insert_session(sqlite3 *db, const char *shot_type,
                      int fgm, int fga, const char *notes);

int db_update_session(sqlite3 *db, int id, const char *shot_type,
                      int fgm, int fga, const char *notes);

int db_delete_session(sqlite3 *db, int id);

int db_get_session(sqlite3 *db, int id, Session *out);

int db_get_all_sessions(sqlite3 *db, Session *out, int capacity);

int db_get_session_by_type(sqlite3 *db, const char *shot_type, 
                           Session *out, int capacity);

#endif
