#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

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

static void report_stats(sqlite3 *db, const char *type)
{
    Session ss[MAX_SESSIONS];

    int count = db_get_session_by_type(db, type, ss, MAX_SESSIONS);

    if (count < 0)
    {
        printf("  Error fetching sessions.\n");
        return;
    }

    char label[64];

    if (type)
    {
        snprintf(label, sizeof(label), "shot type = %s", type);
    }

    else
    {
        snprintf(label, sizeof(label), "%s", "all shot types");
    }

    print_stats(ss, count, label);
}

static void report_breakdown(sqlite3 *db)
{
    const char *types[] = 
    {
        SHOT_TYPE_LAY,
        SHOT_TYPE_MID,
        SHOT_TYPE_3PT
    };

    int ntypes = 3;

    printf("\n  FG%% Breakdown by Shot Type\n");
    printf("  %-10s | %5s | %5s | %6s\n",
           "Type",
           "FGM",
           "FGA",
           "FG%");
    printf("  -----------|-------|-------|-------\n");

    for (int t = 0; t < ntypes; t++)
    {
        Session ss[MAX_SESSIONS];

        int count = db_get_session_by_type(db, types[t], ss, MAX_SESSIONS);

        if (count <= 0)
        {
            printf("  %-10s | %5s | %5s | %6s\n",
                   types[t],
                   "-",
                   "-",
                   "-");
            continue;
        }

        int total_fgm = 0;
        int total_fga = 0;

        for (int i = 0; i < count; i++)
        {
            total_fgm += ss[i].fgm;
            total_fga += ss[i].fga;
        }

        double pct = (total_fga > 0) ? 
            (double)total_fgm / (double)total_fga * 100.0 : 0.0;

        printf("  %-10s | %5d | %5d | %5.1f%%\n",
               types[t],
               total_fgm,
               total_fga,
               pct);
    }

    printf("\n");
}

typedef struct
{
    int show_stats;
    int shot_breakdown;
    int show_history;
    const char *type;
    int last_n;
} Args;

static int parse_args(int argc, char *argv[], Args *out)
{
    struct arg_lit *a_stats = 
        arg_lit0(NULL, 
                "stats", 
                "show overall stats");

    struct arg_lit *a_breakdown =
        arg_lit0(NULL, 
                "breakdown",
                "show FG%% by type");
    
    struct arg_lit *a_history = 
        arg_lit0(NULL, 
                "history", 
                "list session history");
    
    struct arg_str *a_type = 
        arg_str0(NULL,
                "type",
                "<type>",
                "filter by shot type: layup, midrange, 3pt");

    struct arg_int *a_last =
        arg_int0(NULL,
                "last",
                "<N>",
                "show last N sessions (use with --history)");
    
    struct arg_lit *a_help = 
        arg_lit0("h",
                 "help",
                 "print this help and exit");

    struct arg_end *end = arg_end(20);

    void *argtable[] = 
    {
        a_stats,
        a_breakdown,
        a_history,
        a_type,
        a_last,
        a_help,
        end
    };

    int result = 0;
    
    if (arg_nullcheck(argtable) != 0)
    {
        fprintf(stderr, "parse_int:\n\ninsufficient memory.\n");
        result = -1;
    }
    else
    {
        a_last->ival[0] = 0;

        int nerrors = arg_parse(argc, argv, argtable);

        if (a_help->count > 0)
        {
            printf("Usage: fgreport");
            arg_print_syntax(stdout, argtable, "\n");

            printf("\nField goal session reporter.\n\n");

            arg_print_glossary(stdout, argtable, "  %-20s %s\n");

            result = 1;
        }

        else if (nerrors > 0)
        {
            arg_print_errors(stderr, end, "fgreport");

            fprintf(stderr, "Try 'fgreport --help' for more information.\n");
            result = -1;
        }

        else if (a_stats->count == 0 &&
                 a_breakdown->count == 0 &&
                 a_history->count == 0)
        {
            fprintf(stderr, 
                    "fgreport: specify at least one of "
                    "--stats, --breakdown, --history.\n");
            fprintf(stderr, 
                    "Try 'fgreport --help' for information.\n");
            result = -1;
        }
        else
        {
            const char *type = (a_type->count > 0) ? a_type->sval[0] : NULL;

            if (type
                    && strcmp(type, SHOT_TYPE_LAY) != 0
                    && strcmp(type, SHOT_TYPE_MID) != 0
                    && strcmp(type, SHOT_TYPE_3PT) != 0)
            {
                fprintf(stderr,
                        "fgreport: invalid --type '%s'."
                        "Use: layup, midrange, 3pt\n",
                        type);
                result = -1;
            }
            else
            {
                out->show_stats = a_stats->count > 0;
                out->shot_breakdown = a_breakdown->count > 0;
                out->show_history = a_history->count > 0;
                out->type = type;
                out->last_n = a_last->ival[0];
            }
        }
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return result;
}

int main(int argc, char *argv[]);
