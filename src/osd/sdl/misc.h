/* Miscelancelous utility functions

   Copyright 2000 Hans de Goede
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef __MISC_H
#define __MISC_H
#include <stdio.h>

/* clock stuff */
typedef long uclock_t;
uclock_t uclock(void);
#define UCLOCKS_PER_SEC 1000000

/* print colum stuff */
void print_colums(const char *text1, const char *text2);
void fprint_colums(FILE *f, const char *text1, const char *text2);

#if !(defined __USE_BSD || defined __USE_ISOC99 || defined __USE_UNIX98)
int snprintf(char *s, size_t maxlen, const char *fmt, ...);
#endif


#endif /* ifndef __MISC_H */
