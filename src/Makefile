
TARGET = wifiSmartAc

OBJS = main.o smartac.o smartac_commandline.o \
       smartac_config.o smartac_debug.o \
       smartac_do_command.o smartac_http.o \
       smartac_ping.o smartac_post_result.o \
       smartac_remotecmd.o smartac_resultqueue.o \
       smartac_safe.o smartac_util.o \
       smartac_version.o smartac_json_util.o

TARGET_WDCTL = wifiSmartac_update_doglist
TARGET_WDCTL_OBJS = smartac_wdctl_update_list.o smartac_debug.o

all: $(TARGET)  $(TARGET_WDCTL)

$(TARGET): $(OBJS)
	$(CC)  -lpthread   -o     $@   $(OBJS)


$(TARGET_WDCTL): $(TARGET_WDCTL_OBJS)
	$(CC)  -o  $@  $(TARGET_WDCTL_OBJS)

%.o: %c
	$(CC) $< -c  $@
	
clean:
	rm -f *.o $(TARGET) $(TARGET_WDCTL)
	
install:
	@echo "you should be copy the bin file by yourself."
	
