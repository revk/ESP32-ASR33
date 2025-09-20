all: png stl

stl:    ASR33/ASR33T.stl ASR33WAGO/ASR33T.stl ASR33/ASR33B.stl ASR33WAGO/ASR33B.stl

png:	$(patsubst %.kicad_pcb,%.png,$(wildcard */*.kicad_pcb))

PCBCase/case: PCBCase/case.c
	make -C PCBCase

PCBCase/clean: PCBCase/clean.c
	make -C PCBCase

%.stl: %.scad
	echo "Making $@"
	/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD $< -o $@ --backend CGAL
	echo "Made $@"

ASR33/ASR33T.scad: ASR33/ASR33.kicad_pcb PCBCase/case
	PCBCase/case -o $@ $< --edge=2 --bottom=2 --top=7 --pcb=2 --two-file

ASR33WAGO/ASR33T.scad: ASR33WAGO/ASR33.kicad_pcb PCBCase/case
	PCBCase/case -o $@ $< --edge=2 --bottom=2 --top=7 --pcb=2 --two-file

%.png:	%.kicad_pcb PCBCase/clean PCBCase/render
	PCBCase/render $<
