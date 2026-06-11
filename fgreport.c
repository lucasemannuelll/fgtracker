#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <time.h>
#include "db.h"
#include "models.h"
#include "vendor/argtable3.h"

#define MAX_SESSIONS 1024

static const char* time_filter_sql(int opt_week, int opt_month, int opt_year)
{
    if (opt_week)
    {
        return "datetime('now', '-7 days')";
    }
    if (opt_month)
    {
        return "datetime('now', '-1 month')";
    }
    if (opt_year)
    {
        return "datetime('now', '-1 year')";
    }
    return NULL;
}

static void report_history(sqlite3 *db, int last_n, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
    {
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    }
    else
    {
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    }
    
    if (count <= 0)
    {
        printf("  No sessions found.\n");
        return;
    }
    
    int start = 0;
    if (last_n > 0 && last_n < count)
    {
        start = count - last_n;
    }
    
    printf("\n  %-4s | %-19s | %7s | %6s\n", "ID", "Date/time", "FGM/FGA", "FG%%");
    printf("  -----|---------------------|---------|-------\n");
    
    for (int i = start; i < count; i++)
    {
        printf("  %-4d | %-19s | %3d/%-3d | %5.1f%%\n",
               ss[i].id, ss[i].datetime, ss[i].fgm, ss[i].fga, ss[i].fg_pct);
    }
    printf("\n");
}

static void report_stats(sqlite3 *db, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
    {
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    }
    else
    {
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    }
    
    if (count <= 0)
    {
        printf("  No sessions for the selected period.\n");
        return;
    }
    
    int total_fgm = 0, total_fga = 0;
    double best_pct = -1.0, worst_pct = 101.0;
    int best_id = -1, worst_id = -1;
    double sum_fgm = 0.0, sum_fga = 0.0;
    
    for (int i = 0; i < count; i++)
    {
        total_fgm += ss[i].fgm;
        total_fga += ss[i].fga;
        sum_fgm += ss[i].fgm;
        sum_fga += ss[i].fga;
        
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
    
    double overall_pct = (total_fga > 0) ? (double)total_fgm / total_fga * 100.0 : 0.0;
    double avg_fgm = sum_fgm / count;
    double avg_fga = sum_fga / count;
    
    printf("\n=== Career Stats (sessions: %d) ===\n", count);
    printf("  Total FGM       : %d\n", total_fgm);
    printf("  Total FGA       : %d\n", total_fga);
    printf("  Overall FG%%     : %.1f%%\n", overall_pct);
    printf("  Best session    : [%d] %.1f%%\n", best_id, best_pct);
    printf("  Worst session   : [%d] %.1f%%\n", worst_id, worst_pct);
    printf("  Avg makes/session : %.1f\n", avg_fgm);
    printf("  Avg attempts/session : %.1f\n", avg_fga);
    printf("\n");
}

static void report_histogram(sqlite3 *db, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
    {
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    }
    else
    {
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    }
    
    if (count <= 0)
    {
        printf("  No sessions found.\n");
        return;
    }
    
    int bins[10] = {0};
    
    for (int i = 0; i < count; i++)
    {
        int pct = (int)(ss[i].fg_pct / 10.0);
        if (pct < 0)
        {
            pct = 0;
        }
        if (pct > 9)
        {
            pct = 9;
        }
        bins[pct]++;
    }
    
    printf("\n=== FG%% Histogram (10%% bins) ===\n");
    
    for (int i = 0; i < 10; i++)
    {
        int low = i * 10;
        int high = low + 9;
        if (i == 9)
        {
            high = 100;
        }
        printf("  %3d%%–%3d%% : ", low, high);
        int bar_len = bins[i];
        for (int b = 0; b < bar_len; b++)
        {
            printf("#");
        }
        printf(" (%d)\n", bins[i]);
    }
    printf("\n");
}

static void report_lottery(sqlite3 *db)
{
    Session all[MAX_SESSIONS];
    int total_sessions = db_get_all_sessions(db, all, MAX_SESSIONS);
    
    if (total_sessions <= 0)
    {
        printf("  No sessions to pick from.\n");
        return;
    }
    
    srand((unsigned int)time(NULL));
    int idx = rand() % total_sessions;
    Session lucky = all[idx];
    
    int career_fgm = 0, career_fga = 0;
    for (int i = 0; i < total_sessions; i++)
    {
        career_fgm += all[i].fgm;
        career_fga += all[i].fga;
    }
    double career_pct = (career_fga > 0) ? (double)career_fgm / career_fga * 100.0 : 0.0;
    
    double lucky_pct = lucky.fg_pct;
    int hypothetical_fgm = (int)((double)career_fga * lucky_pct / 100.0);
    int diff = hypothetical_fgm - career_fgm;
    
    printf("\n=== YOUR LOTTERY SESSION ===\n");
    printf("  [%d] %s  (%d/%d = %.1f%%)\n",
           lucky.id, lucky.datetime, lucky.fgm, lucky.fga, lucky_pct);
    printf("\n  If ALL your %d sessions had been %.1f%%:\n", total_sessions, lucky_pct);
    printf("    Total FGM would be %d (instead of %d)\n", hypothetical_fgm, career_fgm);
    printf("    Total FGA would be %d (same)\n", career_fga);
    printf("    Career FG%% would be %.1f%% (instead of %.1f%%)\n", lucky_pct, career_pct);
    
    if (diff > 0)
    {
        printf("    You would have +%d more made shots!\n", diff);
    }
    else if (diff < 0)
    {
        printf("    You would have %d fewer made shots.\n", -diff);
    }
    else
    {
        printf("    No difference.\n");
    }
    printf("\n");
}

typedef struct {
    int show_history;
    int show_stats;
    int show_histogram;
    int show_lottery;
    int last_n;
    int filter_week;
    int filter_month;
    int filter_year;
} Args;

static int parse_args(int argc, char *argv[], Args *out)
{
    struct arg_lit *a_history = arg_lit0(NULL, "history", "list all sessions (oldest first)");
    struct arg_lit *a_stats   = arg_lit0(NULL, "stats",   "career statistics");
    struct arg_lit *a_hist    = arg_lit0(NULL, "histogram", "FG% distribution (10% bins)");
    struct arg_lit *a_lottery = arg_lit0(NULL, "lottery", "pick a random session & hypothetical");
    struct arg_int *a_last     = arg_int0(NULL, "last", "<N>", "show last N sessions (with --history)");
    struct arg_lit *a_week     = arg_lit0(NULL, "week",  "filter by last 7 days");
    struct arg_lit *a_month    = arg_lit0(NULL, "month", "filter by last 30 days");
    struct arg_lit *a_year     = arg_lit0(NULL, "year",  "filter by last 365 days");
    struct arg_lit *a_help     = arg_lit0("h", "help", "show help");
    struct arg_end *end = arg_end(20);
    void *argtable[] = { a_history, a_stats, a_hist, a_lottery, a_last,
                         a_week, a_month, a_year, a_help, end };
    
    int result = 0;
    if (arg_nullcheck(argtable) != 0)
    {
        fprintf(stderr, "parse_args: insufficient memory\n");
        result = -1;
    }
    else
    {
        a_last->ival[0] = 0;
        int nerrors = arg_parse(argc, argv, argtable);
        
        if (a_help->count)
        {
            printf("Usage: fgreport [--history] [--stats] [--histogram] [--lottery] ");
            printf("[--last N] [--week] [--month] [--year]\n");
            arg_print_glossary(stdout, argtable, "  %-20s %s\n");
            result = 1;
        }
        else if (nerrors)
        {
            arg_print_errors(stderr, end, "fgreport");
            fprintf(stderr, "Try 'fgreport --help' for more information.\n");
            result = -1;
        }
        else
        {
            int any = a_history->count + a_stats->count + a_hist->count + a_lottery->count;
            if (any == 0)
            {
                fprintf(stderr, "fgreport: specify at least one of --history, --stats, --histogram, --lottery\n");
                result = -1;
            }
            else
            {
                out->show_history   = a_history->count > 0;
                out->show_stats     = a_stats->count > 0;
                out->show_histogram = a_hist->count > 0;
                out->show_lottery   = a_lottery->count > 0;
                out->last_n         = a_last->count ? a_last->ival[0] : 0;
                out->filter_week    = a_week->count > 0;
                out->filter_month   = a_month->count > 0;
                out->filter_year    = a_year->count > 0;
                
                int filters = out->filter_week + out->filter_month + out->filter_year;
                if (filters > 1)
                {
                    fprintf(stderr, "fgreport: use only one of --week, --month, --year\n");
                    result = -1;
                }
            }
        }
    }
    
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
    return result;
}

int main(int argc, char *argv[])
{
    Args args = {0};
    int parsed = parse_args(argc, argv, &args);
    
    if (parsed == 1)
    {
        return 0;
    }
    if (parsed < 0)
    {
        return 1;
    }
    
    sqlite3 *db;
    if (db_open(&db, DB_PATH) < 0)
    {
        return 1;
    }
    
    const char *tf = time_filter_sql(args.filter_week, args.filter_month, args.filter_year);
    
    if (args.show_history)
    {
        report_history(db, args.last_n, tf);
    }
    if (args.show_stats)
    {
        report_stats(db, tf);
    }
    if (args.show_histogram)
    {
        report_histogram(db, tf);
    }
    if (args.show_lottery)
    {
        report_lottery(db);
    }
    
    db_close(db);
    return 0;
}
