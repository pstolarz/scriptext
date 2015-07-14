/*
   Copyright (c) 2015 Piotr Stolarz
   scriptext: Various scripting utilities WinDbg extension

   Distributed under the GNU General Public License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SCRIPTEXT_COMMON_H__
#define __SCRIPTEXT_COMMON_H__

#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <dbgeng.h>
#include "rdflags.h"

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE !FALSE
#endif

#ifndef NULL
#define NULL 0
#endif

#define RNDUP_DW(x)   ((((x)+3)>>2)<<2)

/* TLS and dbg client related functions */
void set_tls_i(DWORD tls_i);
DWORD get_tls_i(void);

/* Get/set debug client object */
void set_client(PDEBUG_CLIENT4 Client);
PDEBUG_CLIENT4 get_client(void);

/* dbg printf related functions */
void dbgprintf(const char *format, ...);
void dbg_dbgprintf(const char *format, ...);
void info_dbgprintf(const char *format, ...);
void warn_dbgprintf(const char *format, ...);
void err_dbgprintf(const char *format, ...);
void cdbgprintf(const char *format, ...);

/* Memory access functions */
ULONG read_memory(ULONG64 addr, PVOID p_buf, ULONG buf_sz, PULONG p_cb);
ULONG write_memory(ULONG64 addr, PVOID p_buf, ULONG buf_sz, PULONG p_cb);

/* Expression evaluation */
BOOL get_expression(PCSTR pc_expr, ULONG64 *p_val, PCSTR *ppc_rem);

/* Replace escaped chars in string 'pc_in'; returns number of processed chars.
   'p_lstc' gets last processed char ending the resulting string (\0 or 'endc').
 */
size_t stresc(char *pc_in, char endc=0, char *p_lstc=NULL);

#endif /* __SCRIPTEXT_COMMON_H__ */
