# wiiosc/Makefile

CC:=gcc
# DEFS:=-D_ENABLE_TILT -D_ENABLE_FORCE 
#-D_DISABLE_NONBLOCK_UPDATES -D_DISABLE_AUTO_SELECT_DEVICE
# CFLAGS:=-Os -Wall -pipe $(DEFS) -g -O2
CFLAGS:=-Os -Wall -pipe -g -O2
# INCLUDES:=-I/usr/include/libcwiid
# LIBS:= -llo -lcwiimote -lbluetooth -lm
LIBS:= -llo -lcwiid -lm

# CFLAGS += -I@top_builddir@/libcwiid
# LDFLAGS += -L@top_builddir@/libcwiid
# LDLIBS += -lcwiid

all: wiiosc

wiiosc: wiiosc.c 
	$(CC) $(CFLAGS) $(INCLUDES) -o wiiosc $< $(LIBS)

clean:
	@rm -f *.o *~ /*
