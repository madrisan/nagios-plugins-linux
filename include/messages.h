#ifndef _MESSAGES_H
#define _MESSAGES_H	1

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Print a message with 'fprintf (stderr, FORMAT, ...)';
   if ERRNUM is nonzero, follow it with ": " and strerror (ERRNUM).
   If STATUS is nonzero, terminate the program with 'exit (STATUS)'.  */

extern void plugin_error (enum nagios_status, int, const char *, ...)
     _attribute_format_printf_(3, 4);

/* This variable is incremented each time 'error' is called.  */
extern unsigned int error_message_count;

extern const char *state_text (enum nagios_status result);

#ifdef __cplusplus
}
#endif

#endif /* _MESSAGES_H */
