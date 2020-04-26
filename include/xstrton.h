/* xstrton.h -- string to double and long conversion with error checking

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef XSTRTON_H_
# define XSTRTON_H_

# ifdef __cplusplus
extern "C" {
# endif

  double strtod_or_err (const char *str, const char *errmesg);
  long strtol_or_err (const char *str, const char *errmesg);

#ifdef __cplusplus
}
#endif

#endif /* XSTRTON_H_ */
