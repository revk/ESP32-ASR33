#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ASR33
SUFFIX := $(shell components/ESP32-RevK/suffix)

all:    asr33 punch asrtweet
	@echo Make: $(PROJECT_NAME)$(SUFFIX).bin
	@idf.py build
	@cp build/$(PROJECT_NAME).bin $(PROJECT_NAME)$(SUFFIX).bin
	@echo Done: $(PROJECT_NAME)$(SUFFIX).bin

set:    wroom solo pico

pico:
	components/ESP32-RevK/setbuildsuffix -S1-PICO
	@make

wroom:
	components/ESP32-RevK/setbuildsuffix -S1
	@make

solo:  
	components/ESP32-RevK/setbuildsuffix -S1-SOLO
	@make

flash:
	idf.py flash

monitor:
	idf.py monitor

clean:
	idf.py clean

menuconfig:
	idf.py menuconfig

pull:
	git pull
	git submodule update --recursive

update:
	git submodule update --init --recursive --remote
	git commit -a -m "Library update"

asr33: asr33.c
	cc -g -O -o asr33 asr33.c -lpopt -lmosquitto

punch: punch.c main/smallfont.h
	cc -g -O -o punch punch.c -lpopt 

AJL/ajl.o: AJL/ajl.c AJL/ajlparse.c
	make -C AJL

asrtweet: asrtweet.c Makefile AJL/ajl.o
	cc -g -O -o $@ $< -I AJL AJL/ajl.o -lpopt -lmosquitto -pthread -lssl -lcrypto

scad: KiCad/ASR33.scad KiCad/ASR33h.scad
stl: KiCad/ASR33.stl KiCad/ASR33h.stl

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@
	echo "Made $@"

KiCad/ASR33.scad: KiCad/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -o $@ $< --edge=2 --base=4.9 --top=7

KiCad/ASR33h.scad: KiCad/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -n -o $@ $< --edge=2 --base=4.9 --top=7
	echo "difference(){top();translate([3,34,-1])cube([45,30,10]);}" >> $@
	echo "translate([spacing,0,0])base();" >> $@

