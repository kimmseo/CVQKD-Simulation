#include <stdarg.h>
#include "path-util.h"

char *fmted_to_string(gchar *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
   
    char *answer = malloc(len + 1);
    va_start(args, fmt); 
    vsnprintf(answer, len + 1, fmt, args);
    va_end(args);

    return answer;
}

/**
 * Set pos and vel of satellites at given time.
 * Copies first half of predict_calc() from predict-tools.c
 */
lw_sat_t sat_at_time(sat_t *sat, gdouble time) {
    lw_sat_t answer;

    sat->jul_utc = time;
    sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

    /* call the norad routines according to the deep-space flag */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, sat->tsince);
    else
        SGP4(sat, sat->tsince);

    Convert_Sat_State(&sat->pos, &sat->vel);
    Magnitude(&sat->vel);

    answer.pos = sat->pos;
    answer.vel = sat->vel;
    answer.jul_utc = sat->jul_utc;

    return answer;
}