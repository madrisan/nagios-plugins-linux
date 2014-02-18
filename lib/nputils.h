#pragma once

#ifndef TRUE
# define TRUE 1
#endif

#ifndef FALSE
# define FALSE 0
#endif

/* Return codes for _set_thresholds */
#define NP_RANGE_UNPARSEABLE 1
#define NP_WARN_WITHIN_CRIT  2

#define OUTSIDE 0
#define INSIDE  1

enum
{
  STATE_OK,
  STATE_WARNING,
  STATE_CRITICAL,
  STATE_UNKNOWN,
  STATE_DEPENDENT
};

/* see: nagios-plugins-1.4.15/lib/utils_base.h */
typedef struct range_struct
{
  double start;
  int start_infinity;		/* FALSE (default) or TRUE */
  double end;
  int end_infinity;
  int alert_on;			/* OUTSIDE (default) or INSIDE */
} range;

typedef struct thresholds_struct
{
  range *warning;
  range *critical;
} thresholds;

int get_status (double, thresholds *);
int set_thresholds (thresholds **, char *, char *);
const char *state_text (int);
void die (int, const char *, ...)
        attribute_noreturn
        attribute_format_printf(2, 3);
