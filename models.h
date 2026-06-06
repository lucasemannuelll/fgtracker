#ifndef MODELS_H
#define MODELS_H

#define SHOT_TYPE_LAY "layup"
#define SHOT_TYPE_MID "midrange"
#define SHOT_TYPE_3PT "3pt"

#define DB_PATH "fgtracker.db"

typedef struct 
{
    int    id;
    char   datetime[32];
    char   shot_type[16];
    int    fgm;
    int    fga;
    double fg_pct;
    char   notes[512];
} Session;

#endif
