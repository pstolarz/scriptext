/*
   Copyright (c) 2015 Piotr Stolarz
   scriptext: Various scripting utilities WinDbg extension

   Distributed under the GNU General Public License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SCRIPTEXT_FILE_H__
#define __SCRIPTEXT_FILE_H__

/* Open a file with name 'pc_file' and a mode 'pc_mode'. Write its handler under
   a pseudo-register with name 'pc_prnm' (in case of file open error 0 will be
   written). Return TRUE on success.
 */
BOOL file_open(
    const char *pc_file, const char *pc_mode, const char *pc_prnm);

/* Write a string/alias value to a file */
BOOL file_wrtstr(FILE *fh, const char *pc_in);

/* Read a line from a file and print it on the console */
void file_rdln(FILE *fh);

#endif /* __SCRIPTEXT_FILE_H__ */
