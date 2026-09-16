#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "db.h"
#include "models.h"

#define MAX_SESSIONS 1024
#define MAX_BAR_WIDTH 50

#define COLOR_RESET "\033[0m"
#define COLOR_HOT "\033[38;5;214m"
#define COLOR_COLD "\033[38;5;39m"
#define COLOR_STEADY "\033[38;5;15m"
#define COLOR_BOLD "\033[1m"

static const char* time_filter_sql(int opt_week, int opt_month, int opt_year)
{
    if (opt_week)
        return "datetime('now', '-7 days')";
    if (opt_month)
        return "datetime('now', '-1 month')";
    if (opt_year)
        return "datetime('now', '-1 year')";

    return NULL;
}

static void report_history(sqlite3 *db, int last_n, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    else
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    
    if (count <= 0)
    {
        printf("  Nenhuma sessão encontrada.\n");
        return;
    }
    
    int start = 0;
    if (last_n > 0 && last_n < count)
        start = count - last_n;
    
    printf("\n  %-4s | %-19s | %7s | %6s\n", "ID", "Data/hora", "FGM/FGA", "FG%%");
    printf("  -----|---------------------|---------|-------\n");
    
    for (int i = start; i < count; i++)
    {
        printf("  %-4d | %-19s | %3d/%-3d | %5.1f%%\n",
                ss[i].id,
                ss[i].datetime,
                ss[i].fgm,
                ss[i].fga,
                ss[i].fg_pct);
    }
    printf("\n");
}

static const char* get_trend(Session *sessions, int total_count, int recent_count)
{
    if (total_count < 3)
        return NULL;

    /* --- Overall (weighted): total makes / total attempts --- */
    int total_fgm = 0, total_fga = 0;
    for (int i = 0; i < total_count; i++)
    {
        total_fgm += sessions[i].fgm;
        total_fga += sessions[i].fga;
    }
    double overall_avg = (total_fga > 0)
                            ? (double)total_fgm / total_fga * 100.0
                            : 0.0;

    /* --- Recent (weighted): last N sessions --- */
    int n = (recent_count < total_count) ? recent_count : total_count;
    int recent_fgm = 0, recent_fga = 0;
    for (int i = total_count - n; i < total_count; i++)
    {
        recent_fgm += sessions[i].fgm;
        recent_fga += sessions[i].fga;
    }
    double recent_avg = (recent_fga > 0)
                            ? (double)recent_fgm / recent_fga * 100.0
                            : 0.0;

    double diff = recent_avg - overall_avg;

    if (diff > 5.0)
        return "↑ EM ALTA";
    else if (diff < -5.0)
        return "↓ EM BAIXA";
    else
        return "= NA MÉDIA";
}

static const char* get_trend_color(const char* trend)
{
    if (trend == NULL)
        return COLOR_RESET;
    
    if (strcmp(trend, "↑ EM ALTA") == 0)
        return COLOR_HOT;
    else if (strcmp(trend, "↓ EM BAIXA") == 0)
        return COLOR_COLD;
    else if (strcmp(trend, "= NA MÉDIA") == 0)
        return COLOR_STEADY;
    
    return COLOR_RESET;
}

static void report_stats(sqlite3 *db, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    else
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    
    if (count <= 0)
    {
        printf("  Nenhuma sessão para o período selecionado.\n");
        return;
    }
    
    int total_fgm = 0, total_fga = 0;
    double best_pct = -1.0, worst_pct = 101.0;
    int best_id = -1, worst_id = -1;
    int best_fgm = -1, best_fga = -1;
    int worst_fgm = -1, worst_fga = -1;
    double sum_fgm = 0.0, sum_fga = 0.0;
    
    for (int i = 0; i < count; i++)
    {
        total_fgm += ss[i].fgm;
        total_fga += ss[i].fga;
        sum_fgm += ss[i].fgm;
        sum_fga += ss[i].fga;
        
        /* Best: higher pct wins; on tie, larger sample is more impressive */
        if (ss[i].fg_pct > best_pct ||
            (ss[i].fg_pct == best_pct && ss[i].fga > best_fga))
        {
            best_pct = ss[i].fg_pct;
            best_fgm = ss[i].fgm;
            best_fga = ss[i].fga;
            best_id = ss[i].id;
        }

        /* Worst: lower pct loses; on tie, larger sample is more damning */
        if (ss[i].fg_pct < worst_pct ||
            (ss[i].fg_pct == worst_pct && ss[i].fga > worst_fga))
        {
            worst_pct = ss[i].fg_pct;
            worst_fgm = ss[i].fgm;
            worst_fga = ss[i].fga;
            worst_id = ss[i].id;
        }
    }
    
    double overall_pct = (total_fga > 0) 
                            ? (double)total_fgm / total_fga * 100.0 
                            : 0.0;
    double avg_fgm = sum_fgm / count;
    double avg_fga = sum_fga / count;

    printf("\n=== Estatísticas Gerais (sessões: %d) ===\n", count);
    printf("  Total de FGM: %d\n", total_fgm);
    printf("  Total de FGA: %d\n", total_fga);
    printf("  FG%% Geral: %.1f%%\n", overall_pct);
    printf("  Melhor sessão: [%d] %d/%d (%.1f%%)\n", best_id, best_fgm, best_fga, best_pct);
    printf("  Pior sessão: [%d] %d/%d (%.1f%%)\n", worst_id, worst_fgm, worst_fga, worst_pct);
    printf("  Média de acertos/sessão: %.1f\n", avg_fgm);
    printf("  Média de tentativas/sessão: %.1f\n", avg_fga);
    
    // Calculate trend (weighted)
    const char* trend = get_trend(ss, count, 5);
    if (trend == NULL)
    {
        printf("  Tendência de arremesso: Dados insuficientes para análise de tendência (mínimo de 3 sessões)\n");
    }
    else
    {
        // Recent weighted average: last 5 sessions
        int n = (5 < count) ? 5 : count;
        int recent_fgm = 0, recent_fga = 0;
        for (int i = count - n; i < count; i++)
        {
            recent_fgm += ss[i].fgm;
            recent_fga += ss[i].fga;
        }
        double recent_avg = (recent_fga > 0)
                                ? (double)recent_fgm / recent_fga * 100.0
                                : 0.0;

        // Overall weighted average == overall_pct already computed above
        const char* color = get_trend_color(trend);
        printf("  Tendência de arremesso: %s%s%s (Média recente: %.1f%% vs geral: %.1f%%)\n",
               color, trend, COLOR_RESET, recent_avg, overall_pct);
    }
    printf("\n");
}

static void report_histogram(sqlite3 *db, const char *time_filter)
{
    Session ss[MAX_SESSIONS];
    int count;
    
    if (time_filter)
        count = db_get_sessions_filtered(db, ss, MAX_SESSIONS, time_filter);
    else
        count = db_get_all_sessions(db, ss, MAX_SESSIONS);
    
    if (count <= 0)
    {
        printf("  Nenhuma sessão encontrada.\n");
        return;
    }
    
    int bins[10] = {0};
    
    for (int i = 0; i < count; i++)
    {
        int pct = (int)(ss[i].fg_pct / 10.0);

        if (pct < 0) pct = 0;
        if (pct > 9) pct = 9;

        bins[pct]++;
    }

    int max_count = 0;
    for (int i = 0; i < 10; i++)
    {
        if (bins[i] > max_count)
            max_count = bins[i];
    }
    
    printf("\n=== Histograma de FG%% (intervalos de 10%%) ===\n");
    for (int i = 0; i < 10; i++)
    {
        int low = i * 10;
        int high = low + 9;
        
        if (i == 9) high = 100;

        printf("  %3d%%–%3d%% : ", low, high);
        
        int bar_len;
        if (max_count > 0)
        {
            bar_len = (bins[i] * MAX_BAR_WIDTH) / max_count;
            if (bar_len < 1 && bins[i] > 0)
                bar_len = 1;
        }
        else bar_len = 0;

        for (int j = 0; j < bar_len; j++)
        {
            printf("#");
        }
        
        printf(" (%d)\n", bins[i]);
    }
    printf("\n");
}

typedef struct {
    int show_history;
    int show_stats;
    int show_histogram;
    int last_n;
    int filter_week;
    int filter_month;
    int filter_year;
} Args;

static void print_usage(const char *prog)
{
    printf("Uso: %s [--history] [--stats] [--histogram] [--last N] "
           "[--week] [--month] [--year] [--help]\n", prog);
    printf("Opções:\n");
    printf("  --history       listar todas as sessões (mais antigas primeiro)\n");
    printf("  --stats         estatísticas gerais\n");
    printf("  --histogram     distribuição de FG%% (intervalos de 10%%)\n");
    printf("  --last <N>      mostrar as últimas N sessões (com --history)\n");
    printf("  --week          filtrar pelos últimos 7 dias\n");
    printf("  --month         filtrar pelos últimos 30 dias\n");
    printf("  --year          filtrar pelos últimos 365 dias\n");
    printf("  -h, --help      mostrar esta ajuda\n");
}

static int parse_args(int argc, char *argv[], Args *out)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--history") == 0) 
            out->show_history = 1;
        else if (strcmp(argv[i], "--stats") == 0) 
            out->show_stats = 1;
        else if (strcmp(argv[i], "--histogram") == 0) 
            out->show_histogram = 1;
        else if (strcmp(argv[i], "--week") == 0) 
            out->filter_week = 1;
        else if (strcmp(argv[i], "--month") == 0) 
            out->filter_month = 1;
        else if (strcmp(argv[i], "--year") == 0) 
            out->filter_year = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            return 1;
        else if (strcmp(argv[i], "--last") == 0) 
        {
            if (i + 1 >= argc) 
            {
                fprintf(stderr, "fgreport: --last requer um argumento\n");
                return -1;
            }
            i++;
            char *endptr;
            long val = strtol(argv[i], &endptr, 10);
            if (*endptr != '\0' || val < 0) 
            {
                fprintf(
                    stderr, 
                    "fgreport: --last requer um número inteiro não negativo\n"
                );
                return -1;
            }
            out->last_n = (int)val;
        } 

        else 
        {
            fprintf(stderr, "fgreport: opção desconhecida '%s'\n", argv[i]);
            return -1;
        }
    }

    int filters = out->filter_week + out->filter_month + out->filter_year;
    if (filters > 1)
    {
        fprintf(
            stderr, 
            "fgreport: use apenas uma das opções --week, --month, --year\n"
        );
        return -1;
    }

    int any = out->show_history + out->show_stats + out->show_histogram;

    if (any == 0) return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    Args args = {0};
    int parsed = parse_args(argc, argv, &args);
    
    if (parsed == 1) 
    {
        print_usage(argv[0]);
        return 0;
    }

    if (parsed < 0)
    {
        print_usage(argv[0]);
        return 1;
    }
    
    sqlite3 *db;
    if (db_open(&db, DB_PATH) < 0)
        return 1;
    
    const char *tf = time_filter_sql(
        args.filter_week,
        args.filter_month,
        args.filter_year
    );
    
    if (args.show_history)
        report_history(db, args.last_n, tf);
    if (args.show_stats)
        report_stats(db, tf);
    if (args.show_histogram)
        report_histogram(db, tf);
    
    db_close(db);
    return 0;
}
