#ifndef MEMINFO_H_
# define MEMINFO_H_

#include "config.h"

#define SU(X) ( ((unsigned long long)(X) << 10) >> shift ), units

void meminfo (int);
void swapinfo (void);
void mempaginginfo (unsigned long *pgpgin, unsigned long *pgpgout);
void swappaginginfo (unsigned long *pswpin, unsigned long *pswpout);

#endif
