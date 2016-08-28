CC = arm-none-linux-gnueabi-gcc

ROOT := $(shell pwd)
INCLUDE := $(ROOT)/include
SRC := $(ROOT)

USR_SUB_DIR := $(SRC)/Dcore $(SRC)/dc

USR_LIBS := $(SRC)/libiec101 $(SRC)/libiec104

default:usr

usr:
	@for n in $(USR_SUB_DIR); do $(MAKE) -C $$n ; done

clean:
	@for n in $(USR_LIBS); do $(MAKE) -C $$n clean ; done
	@for n in $(USR_SUB_DIR); do $(MAKE) -C $$n clean; done
	-rm $(ROOT)/bin_arm_dtu/D*
