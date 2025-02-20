CC ?= gcc
INC_PATH ?= /home/i2t-modelv2x/Documents/v200/docs_v2x/SDK/us_v2xcast_sdk/inc
LIB_PATH ?= /home/i2t-modelv2x/Documents/v200/docs_v2x/SDK/us_v2xcast_sdk/lib
LIBS ?= $(wildcard $(LIB_PATH)/*.so) -pthread -lrt -lm
LDFLAGS :=-g -L$(LIB_PATH)
CFLAGS +=-g -I$(INC_PATH)

EXAMPLES=data_transmitter data_receiver video_transmitter video_receiver multihoping

.PHONY: all

all: $(EXAMPLES)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

data_transmitter: data_transmitter_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'

data_receiver: data_receiver_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'

video_transmitter: video_transmitter_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'

video_receiver: video_receiver_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'
	
multihoping: multihoping_main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) -Wl,-rpath,'$$ORIGIN/../../lib'

clean:
	rm -f *~ *.o $(EXAMPLES)
