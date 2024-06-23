/* include/tntrun.h.  Generated from tntrun.h.in by configure.  */
// Header fuer TNT-Runprogramme

/* Eincompilierter Run-Pfad, wird zB vom Runprogramm "hilfe" benoetigt */
#define TNT_RUN_PATH "/usr/local/tnt/bin"

/* Eincompilierter Main-Pfad, wird zB vom Logbuchprogramm benoetigt */
#define TNT_MAIN_PATH "/usr/local/tnt"

/* Makrodefinitionen fuer Enviromentvariablen */
#define GET_CALLSIGN (char *) getenv("CALLSIGN")
#define GET_LOGNAME (char *) getenv("LOGNAME")
#define GET_CALLSSID (char *) getenv("CALLSSID")

