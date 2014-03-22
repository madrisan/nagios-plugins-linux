#ifndef _PROGNAME_H
#define _PROGNAME_H

/* Programs using this file should do the following in main():
     set_program_name (argv[0]);
 */

#ifdef __cplusplus
extern "C" {
#endif

/* String containing name the program is called with.  */
extern const char *program_name;

/* String containing a short version of 'program_name'.  */
extern const char *program_name_short;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
extern void set_program_name (const char *argv0);

#ifdef __cplusplus
}
#endif

#endif /* _PROGNAME_H */
