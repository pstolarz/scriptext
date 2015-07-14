RM=del
LD=link
MAKE=nmake

WINDBG_DIR=C:/Program Files/Debugging Tools for Windows (x64)

INC=-I "$(WINDBG_DIR)/sdk/inc"
CPPFLAGS_CMN=$(INC) -DREGEX_STATIC
CPPFLAGS=$(CPPFLAGS_CMN) -Yucommon.h

LIB_DIR=-LIBPATH:"$(WINDBG_DIR)/sdk/lib/amd64"
LIBS=dbgeng.lib
LDFLAGS=-DLL $(LIB_DIR) $(LIBS)

OBJS = rdflags.obj \
       common.obj  \
       string.obj  \
       file.obj    \
       scriptext.obj

all: scriptext.dll

clean:
	$(RM) *.obj *.dll *.exp *.lib

cleanall: clean
	$(RM) *.pch
	cd ./regex
	$(MAKE) clean
	cd ..

./regex/regex.obj:
	cd ./regex
	$(MAKE)
	cd ..

# precompiled common headers
common.obj: common.cpp
	$(CPP) -c $(CPPFLAGS_CMN) -Yc$*.h $**

scriptext.dll: ./regex/regex.obj $(OBJS)
	$(LD) -DEF:scriptext.def $(LDFLAGS) $** /out:$@
