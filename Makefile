EXEC = devctl-1.0
OBJS = main.o  protocol.o \
		server.o thread.o \
		utils/util.o utils/string_util.o \
		fee_indicator.o railing_machine.o \
		socket.o  drivers/gpio.o \
		drivers/serial.o ini/dictionary.o \
		ini/iniparser.o  config.o \
		log/log.o

CFLAGS += -Iinclude
.PHONY : clean all

all:${EXEC}
${EXEC} : ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} -pthread  -o $@ ${OBJS} 
	@chmod 777 $@

clean :
	@-rm -f ${OBJS} ${EXEC} *~
