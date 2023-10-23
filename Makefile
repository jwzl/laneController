EXEC = laneController
OBJS = main.o  protocol.o \
		server.o utils/thread.o \
		utils/config.o utils/socket.o \
		utils/util.o utils/string_util.o \
		fee_indicator.o railing_machine.o \
		drivers/gpio.o utils/log.o \
		drivers/serial.o utils/dictionary.o \
		utils/iniparser.o  utils/list.o \
		utils/blockedqueue.o 

client_objs = client.o	protocol.o \
			utils/thread.o utils/socket.o \
			utils/util.o utils/string_util.o \
			utils/log.o utils/list.o \
			utils/blockedqueue.o 

CFLAGS += -Iinclude
.PHONY : clean all

all:${EXEC} client
${EXEC} : ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -pthread  -o $@ ${OBJS} 
	@chmod 777 $@

client: ${client_objs}
	${CC} ${CFLAGS} ${LDFLAGS} -pthread  -o $@ ${client_objs} 
	@chmod 777 $@

clean :
	@-rm -f ${OBJS} ${client_objs} ${EXEC} client *~
