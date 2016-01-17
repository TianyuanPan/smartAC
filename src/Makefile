
TARGET = WifiSmartAc

OBJS = main.o smartac.o smartac_commandline.o \
       smartac_config.o smartac_debug.o \
       smartac_do_command.o smartac_http.o \
       smartac_ping.o smartac_post_result.o \
       smartac_remotecmd.o smartac_resultqueue.o \
       smartac_safe.o smartac_util.o \
       smartac_version.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC)  -lpthread    -o     $@   $(OBJS)

%.o: %c
	$(CC) $< -c  $@
	
clean:
	rm -f *.o $(TARGET)
	
install:
	@echo "you should be copy the bin file by yourself."