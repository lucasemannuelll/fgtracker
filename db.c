#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include "db.h"

static const char *CREATE_TABLE_SQL = 
    "CREATE TABLE IF NOT EXISTS sessions ("
    "   id        INTEGER PRIMARY KEY AUTOINCREMENT,"
    "   datetime  TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),"
    "   fgm       INTEGER NOT NULL,"
    "   fga       INTEGER NOT NULL"
    ");";

int db_open(sqlite3 **db, const char *path)
{
    int rc = sqlite3_open(path, db);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr,
            "db_open: cannot open database: %s\n",
            sqlite3_errmsg(*db)
        );
        sqlite3_close(*db);
        return -1;
    }

    char *err = NULL;

    rc = sqlite3_exec(*db, CREATE_TABLE_SQL, NULL, NULL, &err);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "db_open: cannot create table: %s\n", err);
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
    s->fgm = sqlite3_column_int(stmt, 2);
    s->fga = sqlite3_column_int(stmt, 3);
    s->fg_pct = (s->fga > 0) ? (double)s->fgm / s->fga * 100.0 : 0.0;
}

int db_insert_session(sqlite3 *db, int fgm, int fga)
{
    const char *sql = "INSERT INTO sessions (fgm, fga) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_insert_session: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, fgm);
    sqlite3_bind_int(stmt, 2, fga);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) 
    {
        fprintf(
            stderr, 
            "db_insert_session: step failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    return 0;
}

int db_update_session(sqlite3 *db, int id, int fgm, int fga)
{
    const char *sql = "UPDATE sessions SET fgm=?, fga=? WHERE id=?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_update_session: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, fgm);
    sqlite3_bind_int(stmt, 2, fga);
    sqlite3_bind_int(stmt, 3, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) 
    {
        fprintf(
            stderr, 
            "db_update_session: step failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    if (sqlite3_changes(db) == 0) 
    {
        fprintf(stderr, "db_update_session: no row with id = %d\n", id);
        return -1;
    }
    
    return 0;
}

int db_delete_session(sqlite3 *db, int id)
{
    const char *sql = "DELETE FROM sessions WHERE id=?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_delete_session: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    rc = sqlite3_step(stmt);
    
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) 
    {
        fprintf(
            stderr, 
            "db_delete_session: step failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    if (sqlite3_changes(db) == 0) 
    {
        fprintf(stderr, "db_delete_session: no row with id = %d\n", id);
        return -1;
    }
    
    return 0;
}

int db_get_session(sqlite3 *db, int id, Session *out)
{
    const char *sql = "SELECT * FROM sessions WHERE id=?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_get_session: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    
    if (rc == SQLITE_ROW) 
    {
        row_to_session(stmt, out);
        sqlite3_finalize(stmt);
        return 0;
    }
    
    sqlite3_finalize(stmt);
    return -1;
}

int db_get_all_sessions(sqlite3 *db, Session *out, int capacity)
{
    const char *sql = "SELECT * FROM sessions ORDER BY datetime ASC;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_get_all_sessions: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    int count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && count < capacity) 
    {
        row_to_session(stmt, &out[count]);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}

int db_get_sessions_filtered(sqlite3 *db, Session *out, int capacity,
                             const char *time_filter)
{
    const char *sql_base = "SELECT * FROM sessions WHERE 1=1 ";
    char sql[512];
    
    if (time_filter == NULL) 
    {
        strcpy(sql, sql_base);
        strcat(sql, "ORDER BY datetime ASC;");
    }
    else 
    {
        snprintf(sql, sizeof(sql), 
                "SELECT * FROM sessions "
                "WHERE datetime >= %s "
                "ORDER BY datetime ASC;", 
                time_filter);
    }
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) 
    {
        fprintf(
            stderr, 
            "db_get_sessions_filtered: prepare failed: %s\n", 
            sqlite3_errmsg(db)
        );
        return -1;
    }
    
    int count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW && count < capacity) 
    {
        row_to_session(stmt, &out[count]);
        count++;
    }
    
    sqlite3_finalize(stmt);
    return count;
}
