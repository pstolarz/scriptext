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
#include "string.h"
#include "file.h"
#include <errno.h>

/* DLL entry point */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL ret=TRUE;

    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        set_tls_i(TlsAlloc());
        ret = (get_tls_i()!=TLS_OUT_OF_INDEXES);
        break;

    case DLL_PROCESS_DETACH:
        if (get_tls_i()!=TLS_OUT_OF_INDEXES) TlsFree(get_tls_i());
        break;
    }

    return ret;
}

/* Extension initialization */
HRESULT CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;

    return S_OK;
}

#define MAX_PR_NAME     16

/* Get pseudo-reg name from flag arg and write it to the buffer 'pc_pr_name'
   (min MAX_PR_NAME long)
 */
static BOOL get_pr_name(const flag_desc_t *p_flags_dsc, char *pc_pr_name)
{
    BOOL ret=FALSE;

    memset(pc_pr_name, 0, MAX_PR_NAME);
    pc_pr_name[0]='$'; pc_pr_name[1]='t';

    memcpy(pc_pr_name+2,
        p_flags_dsc->pc_arg, min(p_flags_dsc->arg_len, MAX_PR_NAME-2));
    pc_pr_name[MAX_PR_NAME-1]=0;

    int pr_num = atoi(pc_pr_name+2);
    if (!(pr_num>=0 && pr_num<20)) {
        err_dbgprintf("Pseudo-reg number out of the range [0..19].\n");
        goto finish;
    }

    ret=TRUE;
finish:
    return ret;
}

/* sxtr input/pattern/replacement */
HRESULT CALLBACK sxtr(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT ret=E_FAIL;
    set_client(Client);

    flag_desc_t flags_dsc[] = {{'t', TRUE}, {0}};
    size_t rd_sz = read_flags(args, flags_dsc);
    args += rd_sz;

    char pr_name[MAX_PR_NAME];
    if (flags_dsc[0].is_pres) {
        if (!get_pr_name(&flags_dsc[0], pr_name)) goto finish;
        str_extr(args, pr_name);
    } else {
        str_extr(args, NULL);
    }

    ret=S_OK;
finish:
    return ret;
}

/* fopn [-m mode] [-t num] fname */
HRESULT CALLBACK fopn(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT ret=E_FAIL;
    set_client(Client);

    flag_desc_t flags_dsc[] = {{'m', TRUE}, {'t', TRUE}, {0}};
    size_t rd_sz = read_flags(args, flags_dsc);
    args += rd_sz;

    char mode[16] = "r+";
    if (flags_dsc[0].is_pres) {
        memcpy(mode,
            flags_dsc[0].pc_arg, min(flags_dsc[0].arg_len, sizeof(mode)));
        mode[sizeof(mode)-1]=0;
    }

    char pr_name[MAX_PR_NAME];
    if (flags_dsc[1].is_pres) {
        if (!get_pr_name(&flags_dsc[1], pr_name)) goto finish;
    } else {
        /* set default pseudo-reg */
        strcpy(pr_name, "$t0");
    }

    if (file_open(args, mode, pr_name)) ret=S_OK;

finish:
    return ret;
}

/* fwrt hndl input */
HRESULT CALLBACK fwrt(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT ret=E_FAIL;
    set_client(Client);

    ULONG64 fh_val;
    if (!get_expression(args, &fh_val, &args)) goto finish;
    for (; *args && isspace(*args); args++);

    if (fh_val && file_wrtstr((FILE*)fh_val, args)) ret=S_OK;

finish:
    return ret;
}

/* frdl hndl */
HRESULT CALLBACK frdl(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT ret=E_FAIL;
    set_client(Client);

    ULONG64 fh_val;
    if (!get_expression(args, &fh_val, &args)) goto finish;

    if (fh_val) file_rdln((FILE*)fh_val);

    ret=S_OK;
finish:
    return ret;
}

/* fcls hndl */
HRESULT CALLBACK fcls(PDEBUG_CLIENT4 Client, PCSTR args)
{
    HRESULT ret=E_FAIL;
    set_client(Client);

    ULONG64 fh_val;
    if (!get_expression(args, &fh_val, NULL)) goto finish;

    if (!fh_val || fclose((FILE*)fh_val)) {
        err_dbgprintf("File closure error\n");
    } else ret=S_OK;
finish:
    return ret;
}

/* help info */
HRESULT CALLBACK help(PDEBUG_CLIENT4 Client, PCSTR args)
{
    set_client(Client);

    dbgprintf(
"scriptext: Various scripting utilities\n\n"
"sxtr [-t num] input/pattern/replacement\n"
"    Look for a substring of the input string matching the extended POSIX RE pattern.\n"
"    If the pattern matches, the substring is extracted and modified according to\n"
"    the replacement string. The input string may contain alphanumeric characters\n"
"    plus '_' only, or must be enclosed in '' or \"\". If it's enclosed in [], then\n"
"    the input string specifies an alias name containing a string to process.\n"
"    Delimiter character is recognized as the first one after the input string. All\n"
"    the strings may contain escaped characters.\n"
"    -t: If specified, provides a pseudo-reg $t number where the matching result\n"
"        will be set: 0 - not matched, 1 - matched.\n\n"
"fopn [-m mode] [-t num] fname\n"
"    Open a file with a name fname.\n"
"    -m: Open mode (C standard). \"r+\" by default.\n"
"    -t: Pseudo-reg $t number where a handle of the opened file will be written. In\n"
"        case of opening error zero will be written there. If not specified $t0 is\n"
"        taken.\n"
"fwrt hndl input\n"
"    Write the input string to the file with the handle hndl. If the input string\n"
"    is enclosed in [] then it specifies an alias name containing a string to write.\n"
"    The input string may contain escaped characters.\n"
"frdl hndl\n"
"    Read line from a file with the handle hndl. The file shall be opened for read\n"
"    in the text mode.\n"
"fcls hndl\n"
"    Close a file with a handle hndl.\n\n"
"help\n"
"    Display this help.\n");

    return S_OK;
}
