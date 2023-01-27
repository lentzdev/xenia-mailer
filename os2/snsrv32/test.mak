# makefile for snsrv_32.dll

objdir=.
incldir=.
srcdir=.
# no debug version
copts=/O+ /C /Q /Kbcepr /I $(incldir)
# debug version
# copts=/Ti+ /C /Q /Kbcepr /I $(incldir)
cc=icc
link=link386

all: test.exe

$(objdir)\TEST.OBJ: $(srcdir)\TEST.C
        $(cc) $(copts) $(srcdir)\TEST.C

objs = \
        $(objdir)\TEST.OBJ

test.exe: $(objs) test.mak
# no debug version
        $(link) $(objs),test.exe,,os2386 snsrv_32.lib, /a:16 /noi /exepack;
# debug version
#       $(link) $(objs),snsrv_32.dll,,os2386 snsrv_32.dll, /a:16 /noi /exepack /debug;


