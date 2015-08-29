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
#include "file.h"

/* exported; see header for details */
BOOL file_open(
    const char *pc_file, const char *pc_mode, const char *pc_prnm)
{
    BOOL ret=FALSE;
    FILE *fh=NULL;
    IDebugRegisters2 *DebugRegisters=NULL;

    if ((get_client()->QueryInterface(
        __uuidof(IDebugRegisters2), (void **)&DebugRegisters))!=S_OK)
        goto finish;

    fh = fopen(pc_file, pc_mode);
    if (!fh) err_dbgprintf("File opening error: %s\n", strerror(errno));

    DEBUG_VALUE fh_val;
    fh_val.Type = DEBUG_VALUE_INT64;
    fh_val.I64 = (sizeof(fh)<8 ? DEBUG_EXTEND64(fh) : (ULONG64)fh);
    fh_val.Nat = FALSE;

    ULONG pr_i;
    if (DebugRegisters->GetPseudoIndexByName(pc_prnm, &pr_i)==S_OK)
        if (DebugRegisters->SetPseudoValues(
            DEBUG_REGSRC_DEBUGGEE, 1, NULL, pr_i, &fh_val)==S_OK)
                ret=TRUE;

    if (!ret) err_dbgprintf("Pseudo-reg access error\n");
finish:
    if (fh && !ret) fclose(fh);
    if (DebugRegisters) DebugRegisters->Release();
    return ret;
}

/* exported; see header for details */
BOOL file_wrtstr(FILE *fh, const char *pc_in)
{
    BOOL ret=FALSE;
    IDebugControl2 *DebugControl=NULL;

    char buf[3072];     /* default buffer */
    char *pc_ebuf=NULL; /* extra buffer */

    if (get_client()->QueryInterface(
        __uuidof(IDebugControl2), (void **)&DebugControl)!=S_OK) goto finish;

    strncpy(buf, pc_in, sizeof(buf));
    buf[sizeof(buf)-1]=0;

    char *pc_str;
    if (buf[0]=='[')
    {
        /* alias name */
        char *pc_aname = &buf[1];
        stresc(pc_aname, ']', NULL);
        if (!*pc_aname) goto finish;

        pc_str = pc_aname+strlen(pc_aname)+1;
        size_t rem_buf_sz = pc_str-&buf[0];

        /* get alias val size */
        ULONG aval_sz;
        if (DebugControl->GetTextReplacement(
            pc_aname, 0, NULL, 0, NULL, NULL, 0, &aval_sz)!=S_OK) goto finish;

        /* value read */
        aval_sz = RNDUP_DW(aval_sz+1);
        if (aval_sz > rem_buf_sz) {
            if (!(pc_str=pc_ebuf=(char*)malloc(aval_sz))) goto finish;
        }
        if (DebugControl->GetTextReplacement(
            pc_aname, 0, NULL, 0, NULL, pc_str, aval_sz, NULL)!=S_OK)
                goto finish;
    } else
    if (buf[0]=='\'' || buf[0]=='"') {
        /* apostrophed string */
        pc_str = &buf[1];
        stresc(pc_str, buf[0], NULL);
    } else {
        /* string */
        pc_str=buf;
        stresc(pc_str);
    }
    if (!*pc_str) goto finish;

    size_t cb = strlen(pc_str);
    if (fwrite(pc_str, 1, cb, fh)==cb) ret=TRUE;
    else err_dbgprintf("File write error\n");

    fflush(fh);

finish:
    if (pc_ebuf) free(pc_ebuf);
    if (DebugControl) DebugControl->Release();
    return ret;
}

/* exported; see header for details */
void file_rdln(FILE *fh)
{
    char buf[0x100+1];

    for (int i=0, c; (c=fgetc(fh))!=EOF; i++) {
        if (i>=sizeof(buf)-1) {
            /* flush the read buffer */
            i=0;
            buf[sizeof(buf)-1]=0;
            dbgprintf("%s", buf);
        }

        if (c!='\n') buf[i]=(char)c;
        else {
            buf[i]=0;
            dbgprintf("%s\n", buf);
            break;
        }
    }
}
