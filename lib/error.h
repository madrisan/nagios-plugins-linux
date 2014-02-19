#ifndef _ERROR_H
#define _ERROR_H	1

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Print a message with 'fprintf (stderr, FORMAT, ...)';
   if ERRNUM is nonzero, follow it with ": " and strerror (ERRNUM).
   If STATUS is nonzero, terminate the program with 'exit (STATUS)'.  */

extern void plugin_error (int status, int errnum, const char *format, ...)
     attribute_format_printf(3, 4);

/* This variable is incremented each time 'error' is called.  */
extern unsigned int error_message_count;

#ifdef __cplusplus
}
#endif

#endif /* error.h */
