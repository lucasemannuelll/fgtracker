#ifndef MODELS_H
#define MODELS_H

#define DB_PATH "fgtracker.db"

typedef struct 
{
    int    id;
    char   datetime[32];
    int    fgm;
    int    fga;
    double fg_pct;
} Session;

#endif
