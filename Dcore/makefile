
CC = arm-none-linux-gnueabi-gcc

OBJECTS = control.o core.o errorinfo.o fpga.o ipc.o key.o lcd.o \
	led.o loop.o menu.o battery.o remote.o analog.o math.o com.o wldevice.o server.o \
	NtpTime.o telnet.o config.o anet.o adlist.o lib104.o lib101.o

CFLAGS = -Wl -mcpu=arm926ej -s -finput-charset=UTF-8 -fexec-charset=GBK

LIBS = -lrt -lpthread -lm 

HEADER = ../include


all: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) $(CFLAGS) -L../libs -liec101 -liec104 -lconfig  -o Dcore
	cp Dcore ../bin_arm_dtu

$(OBJECTS): %.o: %.c define.h fmacros.h
	$(CC) -std=gnu99 -finput-charset=UTF-8 -fexec-charset=GBK -I $(HEADER) -c $<

clean:
	rm -f *.o Dcore
