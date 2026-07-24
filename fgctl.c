#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "db.h"
#include "models.h"

static void print_session(const Session *s)
{
    printf("  %-4d | %-19s | %3d/%-3d | %5.1f%%\n",
           s->id, s->datetime, s->fgm, s->fga, s->fg_pct);
}

static int prompt_line(const char *prompt, char *buf, size_t size)
{
    if (prompt != NULL && prompt[0] != '\0')
    {
        printf("%s", prompt);
        fflush(stdout);
    }

    if (fgets(buf, (int)size, stdin) == NULL)
    {
        return -1;
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
    }

    return 0;
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

static void action_insert(sqlite3 *db)
{
    int fgm, fga;
    
    printf("\n--- New Session ---\n");
    
    if (prompt_int("  FGM (makes): ", &fgm) < 0)
    {
        return;
    }
    
    if (prompt_int("  FGA (attempts): ", &fga) < 0)
    {
        return;
    }
    
    if (fgm > fga)
    {
        printf("  Error: makes cannot exceed attempts.\n");
        return;
    }
    
    if (db_insert_session(db, fgm, fga) == 0)
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
    // Fixed header alignment
    printf("\n  %-4s | %-19s | %-7s | %s\n", "ID", "Date/time", "FGM/FGA", "FG%");
    printf("  %-4s-+-%-19s-+-%-7s-+-%s\n", "----", "-------------------", "-------", "-----");
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
    
    char *endptr;
    long id_l = strtol(buf, &endptr, 10);
    if (*endptr != '\0') 
    {
        printf("  Invalid ID.\n");
        return;
    }
    int id = (int)id_l;

    Session existing;
    if (db_get_session(db, id, &existing) < 0)
    {
        printf("  No session with this ID (%d).\n", id);
        return;
    }
    
    printf("  Current: ");
    print_session(&existing);
    printf("  (Press [Enter] to keep current value)\n\n");
    
    int fgm = existing.fgm;
    char tmp[32];
    
    printf("  FGM [%d]: ", existing.fgm);
    
    if (prompt_line("", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0')
    {
        fgm = (int)strtol(tmp, NULL, 10);
    }
    
    int fga = existing.fga;
    printf("  FGA [%d]: ", existing.fga);
    
    if (prompt_line("", tmp, sizeof(tmp)) == 0 && tmp[0] != '\0')
    {
        fga = (int)strtol(tmp, NULL, 10);
    }
    
    if (fgm > fga)
    {
        printf("  Error: makes cannot exceed attempts.\n");
        return;
    }
    
    if (db_update_session(db, id, fgm, fga) == 0)
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
    
    if (prompt_line("  Enter session ID to delete: ", buf, sizeof(buf)) < 0)
    {
        return;
    }
    
    char *endptr;
    long id_l = strtol(buf, &endptr, 10);
    if (*endptr != '\0') 
    {
        printf("  Invalid ID.\n");
        return;
    }
    int id = (int)id_l;

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
    
    int running = 1;
    
    while (running)
    {
        print_menu();
        char line[32];
        
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            printf("\n");
            break;
        }
        
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') 
        {
            line[len - 1] = '\0';
        }

        if (line[0] == '\0')
        {
            continue;
        }

        char *endptr;
        long choice_l = strtol(line, &endptr, 10);
        if (*endptr != '\0')
        {
            printf("  Invalid option.\n");
            continue;
        }

        int choice = (int)choice_l;
        
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
