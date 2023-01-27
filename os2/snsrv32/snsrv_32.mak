# makefile for snsrv_32.dll

objdir=.
incldir=.
srcdir=.
# no debug version        - maybe try /Rn (disable /Gt+) -
copts=/Gt+ /O+ /C /Q /Ge- /Kbcepr /DREALTHING /I $(incldir)
# debug version
# copts=/Gt+ /Ti+ /C /Q /Ge- /Kbcepr /DREALTHING /I $(incldir)
cc=icc
link=link386

all: snsrv_32.dll

$(objdir)\SNSRV_32.OBJ: $(srcdir)\SNSRV_32.C
        $(cc) $(copts) $(srcdir)\SNSRV_32.C

objs = \
        $(objdir)\SNSRV_32.OBJ

snsrv_32.dll: $(objs) snsrv_32.mak
# no debug version
        $(link) $(objs),snsrv_32.dll,,os2386 snserver.lib,snsrv_32.def /a:16 /noi /exepack;
# debug version
#       $(link) $(objs),snsrv_32.dll,,os2386 snserver.lib,snsrv_32.def /a:16 /noi /exepack /debug;

snsrv_32.lib: snsrv_32.def
       implib snsrv_32.lib snsrv_32.dll

