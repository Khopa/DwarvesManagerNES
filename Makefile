.PHONY: clean all

.PRECIOUS: *.o

all: DwarvesManager.nes

clean:
	@rm -fv DwarvesManager.s
	@rm -fv DwarvesManager.o
	@rm -fv DwarvesManager.nes
	@rm -fv crt0.o

crt0.o: crt0.s
	ca65 crt0.s

%.o: %.c
	cc65 -Oi $< --add-source
	ca65 $*.s
	rm $*.s

%.nes: %.o crt0.o
	ld65 -C nes.cfg -o $@ crt0.o $< runtime.lib
