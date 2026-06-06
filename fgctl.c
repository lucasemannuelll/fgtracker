#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/types.h>

#include "db.h"
#include "models.h"
#include "vendor/linenoise.h"

// [H E L P E R S]

static void print_session(const Session *s)
{
    printf("  [%d] %s | %-8s | %d/%d | %.1f%% | %s\n",
            s->id,
            s->datetime,
            s->shot_type,
            s->fgm,
            s->fga,
            s->fg_pct,
            s->notes[0] ? s->notes : "-");
}

static int prompt_line(const char *prompt, char *buf, size_t size)
{
    char *line = linenoise(prompt);

    if (line == NULL)
    {
        return -1;
    }
    
    snprintf(buf, size, "%s", line);
    linenoiseFree(line);

    return 0;
}

static int prompt_shot_type(char *out, size_t size)
{
    char buf[64];
    while (1) 
    {
        if (prompt_line("  Shot type (layup / midrange / 3pt): ",
            buf, sizeof(buf)) < 0)
        {
            return -1;
        }

        if (strcmp(buf, SHOT_TYPE_LAY) == 0 ||
            strcmp(buf, SHOT_TYPE_MID) == 0 ||
            strcmp(buf, SHOT_TYPE_3PT) == 0)
        {
            if (strlen(buf) >= size) {
                printf("  Error: shot type too long.\n");
                continue;
            }
            snprintf(out, size, "%s", buf);
            return 0;
        }

        printf("  Invalid type. Enter: layup, midrange or 3pt\n");
    }
}

static int prompt_int(const char *prompt, int *out)
{
    char buf[32];
    while (1) 
    {
        if (prompt_line(prompt, buf, sizeof(buf)) < 0)
        {
            return -1;
        }

        char *end;
        long val = strtol(buf, &end, 10);

        if (end != buf && *end == '\0' && val >= 0)
        {
            *out = (int)val;
            return 0;
        }

        printf("  Please enter a valid non-negative number.\n");
    }
}

// [M E N U  A C T I O N S]

static void action_insert(sqlite3 *db)
{
    char shot_type[16];
    int fgm, fga;
    char notes[512];

    printf("\n--- New Session ---\n");

    if (prompt_shot_type(shot_type, sizeof(shot_type)) < 0)
    {
        return;
    }

    if (prompt_int("  FGM (makes): ", &fgm) < 0) 
    {
        return;
    }

    if (prompt_int("  FGA (attempts): ", &fga)) 
    {
        return;
    }

    if (fgm > fga)
    {
        printf("  Error: makes cannot exceed attempts.\n");
        return;
    }

    if (prompt_line("  Notes (optional, press [Enter] to skip): ",
                notes, sizeof(notes)) < 0)
    {
        return;
    }
    
    if (db_insert_session(db, shot_type, fgm, fga, notes) == 0) 
    {
        printf("  Session saved.\n");
    }
    else
    {
        printf("  Failed to save session.\n");
    }
}

static void action_list(sqlite3 *db)
{
    Session sessions[256];
    int count = db_get_all_sessions(db, sessions, 256);

    if (count < 0)
    {
        printf("  Error fetching sessions.\n");
        return;
    }

    if (count == 0)
    {
        printf("  No sessions saved yet.\n");
        return;
    }

    printf("\n  ID  | Date/time           | Type     | FGM/FGA | FG%%   | Notes\n");
    printf("--------|---------------------|----------|---------|--------|------\n");

    for (int i = 0; i < count; i++)
    {
        print_session(&sessions[i]);
    }

    printf("\n");
}

static void action_edit(sqlite3 *db)
{
    char buf[32];

    printf("\n--- Edit Session ---\n");

    action_list(db);

    if (prompt_line("  Enter session ID to edit: ", buf, sizeof(buf)) < 0)
    {
        return;
    }

    int id = (int)strtol(buf, NULL, 10);

    Session existing;

    if (db_get_session(db, id, &existing) < 0)
    {
        printf("  No session with this ID (%d).\n", id);
        return;
    }

    printf("  Current: ");
    print_session(&existing);
    printf("  (Press [Enter] to keep current value)\n\n");

    char shot_type[16];
    char tmp[32];

    printf("  Shot type [%s]: ", existing.shot_type);
    if (prompt_line("", tmp, sizeof(tmp)) < 0)
    {
        return;
    }

    if (tmp[0] == '\0') 
    {
        snprintf(shot_type, sizeof(shot_type), "%s", existing.shot_type);
    }

    else 
    {
        if (strcmp(tmp, SHOT_TYPE_LAY) != 0 &&
            strcmp(tmp, SHOT_TYPE_MID) != 0 &&
            strcmp(tmp, SHOT_TYPE_3PT) != 0)
        {
            printf("  Invalid shot type.\n");
            return;
        }

        if (strlen(tmp) >= sizeof(shot_type)) {
             printf("  Error: shot type too long.\n");
             return;
        }

        snprintf(shot_type, sizeof(shot_type), "%s", tmp);
    }

    int fgm = existing.fgm;

    printf("  FGM [%d]: ", existing.fgm);
    
    if (prompt_line("", tmp, sizeof(tmp)) < 0) 
    {
        return;
    }

    if (tmp[0] != '\0')
    {
        fgm = (int)strtol(tmp, NULL, 10);
    }

    int fga = existing.fga;
    
    printf("  FGA [%d]: ", existing.fga);
    
    if (prompt_line("", tmp, sizeof(tmp)) < 0) 
    {
        return;
    }

    if (tmp[0] != '\0')
    {
        fga = (int)strtol(tmp, NULL, 10);
    }

    if (fgm > fga)
    {
        printf("  Error: makes cannot exceed attempts.\n");
        return;
    }

    char notes[512];

    snprintf(notes, sizeof(notes), "%s", existing.notes);

    if (prompt_line("", tmp, sizeof(tmp)) < 0) 
    {
        return;
    }

    if (tmp[0] != '\0')
    {
        snprintf(notes, sizeof(notes), "%s", tmp);
    }

    if (db_update_session(db, id, shot_type, fgm, fga, notes) == 0)
    {
        printf("  Session updated.\n");
    }

    else
    {
        printf("  Failed to update session.\n");
    }
}

static void action_delete(sqlite3 *db)
{
    char buf[32];

    printf("\n--- Delete session ---\n");
    
    action_list(db);

    if (prompt_line("  Enter session ID to delete: ",
        buf, sizeof(buf)) < 0)
    {
        return;
    }

    int id = (int)strtol(buf, NULL, 10);

    char confirm[8];

    printf("  Delete session %d? (y/Y): ", id);

    if (prompt_line("", confirm, sizeof(confirm)) < 0)
    {
        return;
    }

    if (confirm[0] != 'y' && confirm[0] != 'Y')
    {
        printf("  Cancelled.\n");
        return;
    }

    if (db_delete_session(db, id) == 0)
    {
        printf("  Session deleted.\n");
    }
    else
    {
        printf("  Failed to delete session.\n");
    }
}

static void print_menu(void) 
{
    printf("\n=== fgctl ===\n");
    printf("  1. Add session\n");
    printf("  2. List sessions\n");
    printf("  3. Edit session\n");
    printf("  4. Delete session\n");
    printf("  5. Exit\n");
    printf("  > ");
}

int main(void)
{
    sqlite3 *db;
    if (db_open(&db, DB_PATH) < 0)
    {
        return 1;
    }

    linenoiseHistorySetMaxLen(64);

    int running = 1;

    while(running)
    {
        print_menu();
        char *line = linenoise("");

        if (line == NULL)
        {
            break;
        }

        int choice = (int)strtol(line, NULL, 10);

        linenoiseFree(line);

        switch (choice) 
        {
            case 1:
                action_insert(db);
                break;
            case 2:
                action_list(db);
                break;
            case 3:
                action_edit(db);
                break;
            case 4:
                action_delete(db);
                break;
            case 5:
                running = 0;
                break;
            default:
                printf("  Invalid option.\n");
                break;
        }
    }

    db_close(db);
    printf("Bye.\n");
    return 0;
}
