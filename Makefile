#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ASR33

include $(IDF_PATH)/make/project.mk


update:
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

asr33: asr33.c
	cc -O -o asr33 asr33.c -lpopt -lmosquitto

punch: punch.c main/smallfont.h
	cc -O -o punch punch.c -lpopt 

AJL/ajl.o: AJL/ajl.c
	make -C AJL

asrtweet: asrtweet.c Makefile AJL/ajl.o
	cc -O -o $@ $< -I AJL AJL/ajl.o -lpopt -lmosquitto -pthread -lssl -lcrypto

