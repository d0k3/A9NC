LD := arm-none-eabi-ld
AS := arm-none-eabi-as
OC := arm-none-eabi-objcopy

all: loader.h

%.o: %.s
	$(AS) -o $@ $<

%.elf: %.o
	$(LD) -T stub.ld -o $@ $<

%.bin: %.elf
	$(OC) -O binary $< $@

loader.h: loader.bin
	@xxd -i $< > $@

.PHONY: clean
clean:
	@rm -f *.bin *.elf *.o *.h
