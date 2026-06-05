#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include "db.h"

static const char *CREATE_TABLE_SQL = 
    "CREATE TABLE IF NOT EXIST sessions ("
    "   id        INTEGER PRIMARY KEY AUTOINCREMENT,"
    "   datetime  TEXT    NOT NULL DEFAULT (datetime('now', 'localtime')),"
    "   shot_type TEXT    NOT NULL,"
    "   fgm       INTEGER NOT NULL,"
    "   fga       INTEGER NOT NULL,"
    "   notes     TEXT    DEFAULT ''"
    ");";

int db_open(sqlite3 **db, const char *path)
{
    int return_code = sqlite3_open(path, db);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_open:\n\ncannot open database: %s\n", 
                                        sqlite3_errmsg(*db));
        sqlite3_close(*db);
        return -1;
    }

    char *err = NULL;
    return_code = sqlite3_exec(*db, CREATE_TABLE_SQL, NULL, NULL, &err);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_open:\n\ncannot create table: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(*db);
        return -1;
    }

    return 0;
}

void db_close(sqlite3 *db)
{
    sqlite3_close(db);
}

static void row_to_session(sqlite3_stmt *stmt, Session *s)
{
    s->id = sqlite3_column_int(stmt, 0);
    
    const char *dt = (const char *)sqlite3_column_text(stmt, 1);
    snprintf(s->datetime, sizeof(s->datetime), "%s", dt ? dt : "");
    
    const char *st = (const char *)sqlite3_column_text(stmt, 2);
    snprintf(s->shot_type, sizeof(s->shot_type), "%s", st ? st : "");

    s->fgm = sqlite3_column_int(stmt, 3);
    s->fga = sqlite3_column_int(stmt, 4);
    s->fg_pct = (s->fga > 0) ? (double)s->fgm / (double)s->fga * 100.0 : 0.0;

    const char *notes = (const char *)sqlite3_column_text(stmt, 5);
    snprintf(s->notes, sizeof(s->notes), "%s", notes ? notes : "");
}

int db_insert_session(sqlite3 *db, const char *shot_type, 
                      int fgm, int fga, const char *notes)
{
    const char *sql = 
        "INSERT INTO sessions (shot_type, fgm, fga, notes) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int return_code = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_insert_session:\n\nprepare failed: %s\n",
                                        sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, shot_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, fgm);
    sqlite3_bind_int(stmt, 3, fga);
    sqlite3_bind_text(stmt, 4, notes ? notes : "", -1, SQLITE_STATIC);

    return_code = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (return_code != SQLITE_DONE)
    {
        fprintf(stderr, "db_insert_session:\n\nstep failed: %s", 
                                        sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

int db_update_session(sqlite3 *db, int id, const char *shot_type, 
                      int fgm, int fga, const char *notes)
{
    const char *sql = 
        "UPDATE sessions SET shot_type=?, fgm=?, fga=?, notes=? "
        "WHERE=id;";
    
    sqlite3_stmt *stmt;

    int return_code = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_update_session:\n\nprepare failed: %s", 
                                        sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, shot_type, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, fgm);
    sqlite3_bind_int(stmt, 3, fga);
    sqlite3_bind_text(stmt, 4, notes ? notes : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, id);

    return_code = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (return_code != SQLITE_DONE)
    {
        fprintf(stderr, "db_update_session:\n\nstep failed: %s", 
                                        sqlite3_errmsg(db));
        return -1;
    }

    if (sqlite3_changes(db) == 0)
    {
        fprintf(stderr, "db_update_session:\n\nno row with id = %d\n", id);
        return -1;
    }

    return 0;
}

int db_delete_session(sqlite3 *db, int id)
{
    const char *sql = "DELETE FROM sessions WHERE id=?;";
    
    sqlite3_stmt *stmt;
    int return_code = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_delete_session:\n\nprepare failed: %s", 
                                        sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    return_code = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (return_code != SQLITE_DONE)
    {
        fprintf(stderr, "db_delete_session:\n\nstep failed: %s", 
                                        sqlite3_errmsg(db));
        return -1;
    }

    if (sqlite3_changes(db) == 0)
    {
        fprintf(stderr, "db_delete_session:\n\nno row with id = %d\n", id);
        return -1;
    }

    return 0;
}

int db_get_session(sqlite3 *db, int id, Session *out)
{
    const char *sql = 
        "SELECT * FROM sessions "
        "WHERE id=?;";

    sqlite3_stmt *stmt;
    int return_code = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (return_code != SQLITE_OK)
    {
        fprintf(stderr, "db_get_session:\n\nprepare failed: %s",
                                        sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    return_code = sqlite3_step(stmt);
    if (return_code == SQLITE_ROW)
    {
        row_to_session(stmt, out);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}