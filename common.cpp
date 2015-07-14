/*
   Copyright (c) 2015 Piotr Stolarz
   scriptext: Various scripting utilities WinDbg extension

   Distributed under the GNU General Public License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include "common.h"
#include <stdarg.h>

/*
    TLS and dbg client related funcs
 */
static DWORD tls_i=TLS_OUT_OF_INDEXES;

void set_tls_i(DWORD tls_i) {
    ::tls_i=tls_i;
}

DWORD get_tls_i(void) {
    return tls_i;
}

void set_client(PDEBUG_CLIENT4 Client) {
    if (tls_i!=TLS_OUT_OF_INDEXES) TlsSetValue(tls_i, Client);
}

PDEBUG_CLIENT4 get_client(void) {
    return (PDEBUG_CLIENT4)
        (tls_i!=TLS_OUT_OF_INDEXES ? TlsGetValue(tls_i) : NULL);
}

static void vdbgprintf(
    ULONG ctrl, ULONG mask, const char *pc_pref, const char *format, va_list args)
{
    IDebugControl *DebugControl=NULL;

    PDEBUG_CLIENT4 Client;
    if (!(Client=get_client())) goto finish;

    if (Client->QueryInterface(
        __uuidof(IDebugControl), (void **)&DebugControl)!=S_OK) goto finish;

    if (pc_pref) DebugControl->ControlledOutput(ctrl, mask, "%s: ", pc_pref);
    DebugControl->ControlledOutputVaList(ctrl, mask, format, args);

finish:
    if (DebugControl) DebugControl->Release();
    return;
}

#define DBGPRNT_METHOD(name, ctrl, pref)      \
    void name(const char *format, ...) {      \
        va_list args;                         \
        va_start(args, format);               \
        vdbgprintf(ctrl, DEBUG_OUTPUT_NORMAL, \
            pref, format, args);              \
        va_end(args);                         \
    }

DBGPRNT_METHOD(dbgprintf, DEBUG_OUTCTL_ALL_CLIENTS, NULL)
DBGPRNT_METHOD(dbg_dbgprintf, DEBUG_OUTCTL_ALL_CLIENTS, "DBG")
DBGPRNT_METHOD(info_dbgprintf, DEBUG_OUTCTL_ALL_CLIENTS, "INFO")
DBGPRNT_METHOD(warn_dbgprintf, DEBUG_OUTCTL_ALL_CLIENTS, "WARN")
DBGPRNT_METHOD(err_dbgprintf, DEBUG_OUTCTL_ALL_CLIENTS, "ERR")
DBGPRNT_METHOD(cdbgprintf, DEBUG_OUTCTL_THIS_CLIENT, NULL)

/*
    Memory access functions
 */
#define MEMACCESS_METHOD(name, func)                                         \
    ULONG name(ULONG64 addr, PVOID p_buf, ULONG buf_sz, PULONG p_cb)         \
    {                                                                        \
        ULONG ret=FALSE;                                                     \
        IDebugDataSpaces *DebugDataSpaces=NULL;                              \
                                                                             \
        PDEBUG_CLIENT4 Client;                                               \
        if (Client=get_client()) {                                           \
            if (Client->QueryInterface(__uuidof(IDebugDataSpaces),           \
                (void **)&DebugDataSpaces)==S_OK) {                          \
                ret=(DebugDataSpaces->func(addr, p_buf, buf_sz, p_cb)==S_OK);\
            }                                                                \
        }                                                                    \
                                                                             \
        if (DebugDataSpaces) DebugDataSpaces->Release();                     \
        return ret;                                                          \
    }

/* WdbgExts ReadMemory(), WriteMemory() analogous */
MEMACCESS_METHOD(read_memory, ReadVirtual)
MEMACCESS_METHOD(write_memory, WriteVirtual)

/* WdbgExts GetExpressionEx() analogous */
BOOL get_expression(PCSTR pc_expr, ULONG64 *p_val, PCSTR *ppc_rem)
{
    BOOL ret=FALSE;
    IDebugControl *DebugControl=NULL;

    PDEBUG_CLIENT4 Client;
    if (!(Client=get_client())) goto finish;

    if ((Client->QueryInterface(
        __uuidof(IDebugControl), (void **)&DebugControl))!=S_OK) goto finish;

    ULONG rem_i;
    DEBUG_VALUE val;
    if (DebugControl->Evaluate(pc_expr, DEBUG_VALUE_INT64, &val, &rem_i)!=S_OK)
        goto finish;

    if (ppc_rem) {
        for (pc_expr+=rem_i; isspace(*pc_expr); pc_expr++);
        *ppc_rem = pc_expr;
    }
    *p_val = (ULONG64)val.I64;

    ret=TRUE;
finish:
    if (DebugControl) DebugControl->Release();
    return ret;
}

#define HEXDIG2INT(c)   \
    ((int)(('0'<=(c) && (c)<='9') ? (c)-'0' : \
    (('A'<=(c) && (c)<='F') ? (c)-'A'+10 : \
    (('a'<=(c) && (c)<='f') ? (c)-'a'+10 : -1))))

/* exported; see header for details */
size_t stresc(char *pc_in, char endc, char *p_lstc)
{
    char c;
    size_t i, j;

    for (i=j=0;
        (c=pc_in[i], (c && c!=endc));
        pc_in[j]=c, i++, j++)
    {
        if (c!='\\') continue;

        char esc=pc_in[i+1];
        switch (esc)
        {
        case 'a': c='\a'; i++; break;
        case 'b': c='\b'; i++; break;
        case 'f': c='\f'; i++; break;
        case 'n': c='\n'; i++; break;
        case 'r': c='\r'; i++; break;
        case 't': c='\t'; i++; break;
        case 'v': c='\v'; i++; break;
        case '\\': c='\\'; i++; break;
        case '\'': c='\''; i++; break;
        case '"': c='"'; i++; break;
        case 'x':
          {
            /* hex encoded char */
            int c1=HEXDIG2INT(pc_in[i+2]);
            if (c1==-1) break;

            int c2=HEXDIG2INT(pc_in[i+3]);
            if (c2==-1) break;

            i+=3;
            c=(c1<<4 | c2);
            break;
          }
        default:
            if (endc && esc==endc) { c=esc; i++; }
            break;
        }
    }

    if (p_lstc) *p_lstc=c;
    pc_in[j]=0;
    return i+1;
}
