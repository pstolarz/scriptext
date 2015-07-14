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
#include "regex/regex.h"

#define RE_GROUPS       10

/* exported; see header for details */
void str_extr(const char *pc_in, const char *pc_prnm)
{
    BOOL b_match=FALSE;
    IDebugControl2 *DebugControl=NULL;
    IDebugRegisters2 *DebugRegisters=NULL;

    char buf[3072];     /* default buffer */
    char *pc_ebuf=NULL; /* extra buffer */

    if (get_client()->QueryInterface(
        __uuidof(IDebugControl2), (void **)&DebugControl)!=S_OK) goto finish;

    /* parse input arguments */
    char delim, lstc;
    char *pc_str=NULL, *pc_aname=NULL;
    char *pc_pttrn, *pc_replcmt;

    strncpy(buf, pc_in, sizeof(buf));
    buf[sizeof(buf)-1]=0;

    if (buf[0]=='\'' || buf[0]=='"')
    {
        /* string in apostrophes  */
        pc_str = &buf[1];
        pc_pttrn = pc_str+stresc(pc_str, buf[0], &lstc);
        if (!lstc || !*pc_str) goto finish;
    } else
    if (buf[0]=='[')
    {
        /* alias name */
        pc_aname = &buf[1];
        pc_pttrn = pc_aname+stresc(pc_aname, ']', &lstc);
        if (!lstc || !*pc_aname) goto finish;
    } else
    {
        /* string of alphanums + '_' */
        size_t i;
        for (i=0, pc_str=&buf[0]; isalnum(pc_str[i]) || pc_str[i]=='_'; i++);
        pc_pttrn=&pc_str[i];
        if (!i) goto finish;
    }

    /* delimiter char */
    delim = *pc_pttrn;
    *pc_pttrn++ = 0;
    if (!delim || delim=='\\') goto finish;

    /* RE pattern */
    pc_replcmt = pc_pttrn+stresc(pc_pttrn, delim, &lstc);
    if (!*pc_pttrn) goto finish;

    /* replacement string */
    if (!lstc) pc_replcmt--;
    stresc(pc_replcmt);

    if (pc_aname)
    {
        pc_str = pc_replcmt+strlen(pc_replcmt)+1;
        size_t rem_buf_sz = pc_str-&buf[0];

        /* get alias val size */
        ULONG aval_sz;
        if (DebugControl->GetTextReplacement(
            pc_aname, 0, NULL, 0, NULL, NULL, 0, &aval_sz)!=S_OK) goto finish;

        /* read value */
        aval_sz = RNDUP_DW(aval_sz+1);
        if (aval_sz > rem_buf_sz) {
            if (!(pc_str=pc_ebuf=(char*)malloc(aval_sz))) goto finish;
        }
        if (DebugControl->GetTextReplacement(
            pc_aname, 0, NULL, 0, NULL, pc_str, aval_sz, NULL)!=S_OK)
            goto finish;

        if (!*pc_str) goto finish;
    }

    regex_t re;
    regmatch_t rms[RE_GROUPS];

    /* compile RE pattern and match with the input string */
    if (regcomp(&re, pc_pttrn, REG_EXTENDED)) goto finish;
    if (!regexec(&re, pc_str, RE_GROUPS, rms, 0))
    {
        b_match=TRUE;

        /* print the replacement string */
        size_t i;
        for (i=0; pc_replcmt[i]; i++)
        {
            if (pc_replcmt[i]!='\\') continue;

            char esc = pc_replcmt[i+1];
            if (!('0'<=esc && esc<='9')) continue;

            /* RE group */
            int grp = esc-'0';
            if (rms[grp].rm_so==-1 || rms[grp].rm_eo==-1) continue;

            if (i) {
                /* print partial replacement */
                pc_replcmt[i] = 0;
                cdbgprintf("%s", pc_replcmt);
            }

            /* print group */
            char *pc_grp_start = &pc_str[rms[grp].rm_so];
            char *pc_grp_end = &pc_str[rms[grp].rm_eo];

            char c = *pc_grp_end;
            *pc_grp_end = 0;
            cdbgprintf("%s", pc_grp_start);
            *pc_grp_end = c;

            /* continue loop from the place after the group mark */
            pc_replcmt = &pc_replcmt[i+2];
            i=(size_t)-1;
        }

        /* print last part of the replacement */
        if (i) cdbgprintf("%s", pc_replcmt);
    }

    regfree(&re);
finish:
    if (pc_prnm) {
        /* if required set the result in pseudo-reg */
        if ((get_client()->QueryInterface(
            __uuidof(IDebugRegisters2), (void **)&DebugRegisters))==S_OK)
        {
            DEBUG_VALUE mtch_val;
            mtch_val.Type = DEBUG_VALUE_INT32;
            mtch_val.I32 = (ULONG)(b_match ? 1 : 0);

            ULONG pr_i;
            if (DebugRegisters->GetPseudoIndexByName(pc_prnm, &pr_i)==S_OK) {
                DebugRegisters->SetPseudoValues(
                    DEBUG_REGSRC_DEBUGGEE, 1, NULL, pr_i, &mtch_val);
            }
        }
    }
    if (pc_ebuf) free(pc_ebuf);
    if (DebugRegisters) DebugRegisters->Release();
    if (DebugControl) DebugControl->Release();
    return;
}
