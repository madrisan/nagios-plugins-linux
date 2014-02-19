#pragma once

/* Return codes for _set_thresholds */
#define NP_RANGE_UNPARSEABLE 1
#define NP_WARN_WITHIN_CRIT  2

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
