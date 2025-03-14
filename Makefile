#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := ASR33
SUFFIX := $(shell components/ESP32-RevK/buildsuffix)
MODELS := ASR33 ASR33h

all:    main/settings.h
	@echo Make: $(PROJECT_NAME)$(SUFFIX).bin
	@idf.py build
	@cp build/$(PROJECT_NAME).bin $(PROJECT_NAME)$(SUFFIX).bin
	@echo Done: $(PROJECT_NAME)$(SUFFIX).bin

beta:
	-git pull
	-git submodule update --recursive
	-git commit -a
	@make set
	cp ASR33*.bin release/beta
	git commit -a -m Beta
	git push

issue:
	make -C PCB
	-git pull
	-git commit -a
	cp -f release/beta/$(PROJECT_NAME)*.bin release
	-git commit -a -m Release
	-git push

main/settings.h:     components/ESP32-RevK/revk_settings main/settings.def components/ESP32-RevK/settings.def
	components/ESP32-RevK/revk_settings $^

components/ESP32-RevK/revk_settings: components/ESP32-RevK/revk_settings.c
	make -C components/ESP32-RevK

tools:	asr33 punch asrtweet

set:    wroom solo pico s3

s3:
	components/ESP32-RevK/setbuildsuffix -S3-MINI-N4-R2
	@make

pico:
	components/ESP32-RevK/setbuildsuffix -S1-PICO
	@make

wroom:
	components/ESP32-RevK/setbuildsuffix -S1-V1
	@make

solo:  
	components/ESP32-RevK/setbuildsuffix -S1-SOLO
	@make

ftdizap/ftdizap: ftdizap/ftdizap.c
	make -C ftdizap

ftdi: ftdizap/ftdizap
	./ftdizap/ftdizap --serial="RevK" --description="Generic" --cbus0-mode=7 --cbus1-mode=13

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
	-git commit -a
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

PCBCase/case: PCBCase/case.c
	make -C PCBCase

scad:   $(patsubst %,PCB/%.scad,$(MODELS))
stl:    $(patsubst %,PCB/%.stl,$(MODELS))

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@
	echo "Made $@"

PCB/ASR33.scad: PCB/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -o $@ $< --edge=2 --base=3 --top=6

PCB/ASR33h.scad: PCB/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -n -o $@ $< --edge=2 --base=2 --top=7
	@echo "difference(){top();translate([3,34,-1])cube([45,30,10]);}" >> $@
	@echo "translate([spacing,0,0])base();" >> $@

