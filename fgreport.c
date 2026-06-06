#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "db.h"
#include "models.h"
#include "vendor/argtable3.h"

#define MAX_SESSIONS 1024

static void print_header(void)
{
    printf("  %-4s | %-19s | %-8s | %7s | %6s | %s\n",
           "ID", "Date/time", "Type", "FGM/FGA", "FG%", "Notes");

    printf("  -----|---------------------|----------|---------|-------|------\n");
}

static void print_row(const Session *s)
{
    printf("  %-4d | %-19s | %-8s | %3d/%-3d | %5.1f%% | %s\n",
           s->id,
           s->datetime,
           s->shot_type,
           s->fgm,
           s->fga,
           s->fg_pct,
           s->notes[0] ? s->notes : "-");
}

static void print_stats(const Session *ss, int count, const char *label)
{
    if (count == 0)
    {
        printf("  No sessions found.\n");
        return;
    }

    int total_fgm = 0;
    int total_fga = 0;

    double best_pct = -1.0;
    double worst_pct = 101.0;

    int best_id = -1;
    int worst_id = -1;

    for (int i = 0; i < count; i++)
    {
        total_fgm += ss[i].fgm;
        total_fga += ss[i].fga;

        if (ss[i].fg_pct > best_pct)
        {
            best_pct = ss[i].fg_pct;
            best_id = ss[i].id;
        }

        if (ss[i].fg_pct < worst_pct)
        {
            worst_pct = ss[i].fg_pct;
            worst_id = ss[i].id;
        }
    }

    double overall_pct = (total_fga > 0) ? 
        (double)total_fgm / (double)total_fga * 100.0 : 0.0;

    printf("\n  Stats: %s\n", label);
    printf("  Sessions  : %d\n", count);
    printf("  Total FGM : %d\n", total_fgm);
    printf("  Total FGA : %d\n", total_fga);
    printf("  Overall FG%%: %.1f%%\n", overall_pct);
    printf("  Best  session [id = %d]: %.1f%%\n", best_id, best_pct);
    printf("  Worst session [id = %d]: %.1f%%\n", worst_id, worst_pct);
    printf("\n");
}


static void report_history(sqlite3 *db, const char *type, int last_n)
{
    Session ss[MAX_SESSIONS];

    int count = db_get_session_by_type(db, type, ss, MAX_SESSIONS);

    if (count < 0)
    {
        printf("  Error fetching sessions.\n");
        return;
    }

    if (count == 0)
    {
        printf("  No sessions found.\n");
        return;
    }

    int start = 0;

    if (last_n > 0 && last_n < count)
    {
        start = count - last_n;
    }

    print_header();

    for (int i = 0; i < count; i++)
    {
        print_row(&ss[i]);
    }

    printf("\n");
}

static void report_stats(sqlite3 *db, const char *type);
static void report_breakdown(sqlite3 *db);

typedef struct
{
    int show_stats;
    int shot_breakdown;
    int show_history;
    const char *type;
    int last_n;
} Args;

static int parse_args(int argc, char *argv[], Args *out);

int main(int argc, char *argv[]);
