/*
   Copyright (c) 2015 Piotr Stolarz
   scriptext: Various scripting utilities WinDbg extension

   Distributed under the GNU General Public License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SCRIPTEXT_STRING_H__
#define __SCRIPTEXT_STRING_H__

/* Extract and modify a pattern from a string. 'pc_in' points to an input in
   the form of: string | '['alias']' D pattern D replacement, where D is a
   delimiter char. If 'pc_prnm' is not NULL the match result is set under the
   indicated pseudo-reg. Return TRUE is success and write the result to the
   client's output.
 */
void str_extr(const char *pc_in, const char *pc_prnm);

#endif /* __SCRIPTEXT_STRING_H__ */
