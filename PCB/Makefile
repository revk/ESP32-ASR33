all: png stl

stl:    ASR33/ASR33.stl ARS33WAGO/ASR33.stl

png:	$(patsubst %.kicad_pcb,%.png,$(wildcard */*.kicad_pcb))

PCBCase/case: PCBCase/case.c
	make -C PCBCase

PCBCase/clean: PCBCase/clean.c
	make -C PCBCase

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@
	echo "Made $@"

ASR33/ASR33.scad: ASR33/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -o $@ $< --edge=2 --bottom=2 --top=7 --pcb=2

ASR33WAGO/ASR33.scad: ASR33WAGO/ASR33.kicad_pcb PCBCase/case Makefile
	PCBCase/case -o $@ $< --edge=2 --bottom=2 --top=7 --pcb=2

%.png:	%.kicad_pcb PCBCase/clean PCBCase/render Makefile
	PCBCase/render $<
