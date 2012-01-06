ifeq ($(ARCH),target)
	PKG_CONFIG_PATH=/usr/lib/pkgconfig
	export PKG_CONFIG_PATH
	CC=arm-linux-gcc -Wall
	CXX=arm-linux-g++ -Wall
else
	PKG_CONFIG_PATH=/usr/lib/pkgconfig
	export PKG_CONFIG_PATH
	CC=gcc -Wall
    CXX=gcc -Wall
endif

TS1=utc_mm_session_init_func
TS2=utc_mm_session_init_ex_func
TS3=utc_mm_session_finish_func

LIBS = `pkg-config --libs mm-session`
LIBS +=-L/usr/lib/:/usr/lib/pkgconfig
LIBS +=$(TET_ROOT)/lib/tet3/tcm_s.o
LIBS +=-L$(TET_ROOT)/lib/tet3 -ltcm_s
LIBS +=-L$(TET_ROOT)/lib/tet3/ -lapi_s

INCS = -I. `pkg-config --cflags  mm-session `
INCS += -I$(TET_ROOT)/inc/tet3
INCS += -I/usr/include/mmf

CFLAGS = $(INCS) 
CC += $(CFLAGS)
LDFLAGS = $(LIBS)

all : $(TS1) $(TS2)
	
$(TS1): $(TS1).c
	$(CC) -o $(TS1) $(TS1).c $(LDFLAGS)

$(TS2): $(TS2).c
	$(CC) -o $(TS2) $(TS2).c $(LDFLAGS)
	
$(TS3): $(TS3).c
	$(CC) -o $(TS3) $(TS3).c $(LDFLAGS)
clean:
	rm -rf *~ *.o $(TS1) $(TS2) $(TS3)
	


