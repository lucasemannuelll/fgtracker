#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "db.h"
#include "models.h"
#include "vendor/linenoise.h"

static void print_session(const Session *s)
{
    printf("  [%d] %s | %-8s | %d/%d | %.1f%% | %s\n",
            s->id, s->datetime, s->shot_type,
            s->fgm, s->fga, s->fg_pct,
            s->notes[0] ? s->notes : "-");
}



static int prompt_line(const char *prompt, char *buf, size_t size);
static int prompt_shot_type(char *out, size_t size);
static int prompt_int(const char *prompt, int *out);
static void action_insert(sqlite3 *db);
static void action_list(sqlite3 *db);
static void action_edit(sqlite3 *db);
static void action_delete(sqlite3 *db);
static void print_menu(void);
