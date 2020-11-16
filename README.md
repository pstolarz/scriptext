WinDbg scripting extensions
===========================

Among many WinDbg scripting language flaws is a complete lack of advanced text
searching/replacing functionality, especially associated with aliases. The library
provides simple, "s" sed like command to search and replace regexp patterns in
strings and WinDbg aliases. Together with WinDbg functionality of setting aliases
to arbitrary WinDbg commands outputs, the library provides an effective tool to
write advanced WinDbg scripts without a need to reach for Python substitutions.

Additionally the library supplies simple set of commands to open, read, write and
close text/binary files. Of course this ain't shell nor Python but for simple
scripts should be sufficient.

NOTE: Since my contact with Windows platform and the WinDbg debugger is currently
rather occasional, the project is no more maintained. If someone is interested
to continue work on it, feel free to contact me.

scriptext help command output
-----------------------------

    scriptext: Various scripting utilities

    sxtr [-t num] input/pattern/replacement
        Look for a substring of the input string matching the extended POSIX RE pattern.
        If the pattern matches, the substring is extracted and modified according to
        the replacement string. The input string may contain alphanumeric characters
        plus '_' only, or must be enclosed in '' or "". If it's enclosed in [], then
        the input string specifies an alias name containing a string to process.
        Delimiter character is recognized as the first one after the input string. All
        the strings may contain escaped characters.
        -t: If specified, provides a pseudo-reg $t number where the matching result
            will be set: 0 - not matched, 1 - matched.

    fopn [-m mode] [-t num] fname
        Open a file with a name fname.
        -m: Open mode (C standard). "r+" by default.
        -t: Pseudo-reg $t number where a handle of the opened file will be written. In
            case of opening error zero will be written there. If not specified $t0 is
            taken.
    fwrt hndl input
        Write the input string to the file with the handle hndl. If the input string
        is enclosed in [] then it specifies an alias name containing a string to write.
        The input string may contain escaped characters.
    frdl hndl
        Read line from a file with the handle hndl. The file shall be opened for read
        in the text mode.
    fcls hndl
        Close a file with a handle hndl.

    help
        Display this help.

Compiling and Installing
------------------------

Prerequisites:

 - MS SDK with `cl`, `link` and `nmake`; no need for MS Visual Studio,
 - Debugging Tools for Windows with its SDK,

Compilation:

Set required building environment depending on your target platform (x86/x64/ia64,
debug/release etc.) by calling MS SDK's `SetEnv.Cmd` script with proper arguments
and make the library:

    nmake

The result is `scriptext.dll` library located in the sources directory. Install
it by:

    nmake install

Loading and testing
-------------------

I. Loading

    0:000> .load scriptext

II. Help info

    0:000> !scriptext.help

III. Text searching and replacement

Create `test_alias` alias, look for `test` string in it and replace by `TEST`.
`$t0` pseudo register is set to 1 (the searched string is found in the alias).

    0:000> aS test_alias "Let's try test"

    0:000> al
      Alias            Value
     -------          -------
     test_alias       Let's try test

    0:000> !sxtr -t0 [test_alias]/(.*)test(.*)/\1TEST\2
    Let's try TEST
    0:000> r @$t0
    $t0=0000000000000001

Look for `test` and `TEST` substrings in `test_alias` and set `$t0` pseudo
register as a result of searching. Searching is performed on the alias
substitution.

    0:000> !sxtr "-t0 \"${test_alias}\"/test"
    0:000> r @$t0
    $t0=0000000000000001

    0:000> !sxtr "-t0 \"${test_alias}\"/TEST"
    0:000> r @$t0
    $t0=0000000000000000

IV. Simple grep

Create the following script with filename `simple_grep`:

    !fopn -m "r" ${$arg1}
    .if (@$t0!=0) {
      aS /c ${/v:ln} !frdl @$t0
      .while (1) {
        !sxtr -t1 [ln]/.+
        .if (@$t1!=0) {
          !sxtr -t1 [ln]/${$arg2}
          .if (@$t1!=0) {
            .echo ${ln}
          }
        } .else {
          .break
        }
        aS /c ${/v:ln} !frdl @$t0
      }
      !fcls @$t0
      ad /q ${/v:ln}
    }

and execute it with 2 arguments: input file and searched RE pattern:

    0:000> $$>a< simple_grep file pattern

License
-------

GNU GENERAL PUBLIC LICENSE v2. See LICENSE file for details.
