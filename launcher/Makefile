# Makefile is originally from CakesFW by b1l1s / mid-kid
# Slightly edited it to fit my purposes

rwildcard = $(foreach d, $(wildcard $1*), $(filter $(subst *, %, $2), $d) $(call rwildcard, $d/, $2))

CC := arm-none-eabi-gcc
AS := arm-none-eabi-as
OC := arm-none-eabi-objcopy

dir_source := source
dir_build  := build
dir_loader := loader

ASFLAGS  := -mlittle-endian -march=armv5te -mcpu=arm946e-s
CFLAGS   := $(ASFLAGS) -g -Os -flto -marm -Wall -Wextra -Wno-main -ffast-math \
			-fshort-wchar -Wall -Wextra -Wpedantic -pedantic -Wall -Wextra -Wcast-align \
			-Wcast-qual -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations \
			-Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wundef -Wno-unused

LDFLAGS  := -nostartfiles -T linker.ld $(LIBDIRS) $(LIBS) -g $(ASFLAGS) 

objects = $(patsubst $(dir_source)/%.s, $(dir_build)/%.o, \
		  $(patsubst $(dir_source)/%.c, $(dir_build)/%.o, \
		  $(call rwildcard, $(dir_source), *.s *.c)))

.PHONY: all
all: arm9loaderhax_si.bin

.PHONY: clean
clean:
	@rm -rf release $(dir_build) arm9loaderhax_si.bin
	@make --no-print-directory -C $(dir_loader) -f Makefile clean
	@echo cleaned ...

arm9loaderhax_si.bin: $(dir_build)/main.elf
	$(OC) -S -O binary $< $@
	@echo built ...

$(dir_build)/main.elf: $(objects)
	$(CC) $^ $(LDFLAGS) $(OUTPUT_OPTION)

$(dir_source)/$(dir_loader).h:
	@make --no-print-directory -C $(dir_loader) -f Makefile
	@cp $(dir_loader)/$(dir_loader).h $@

$(dir_build)/main.o: $(dir_source)/$(dir_loader).h

$(dir_build)/%.o: $(dir_source)/%.c
	@mkdir -p "$(@D)"
	$(COMPILE.c) $(CFLAGS) $(OUTPUT_OPTION) $<

$(dir_build)/%.o: $(dir_source)/%.s
	@mkdir -p "$(@D)"
	$(COMPILE.s) $(ASFLAGS) $(OUTPUT_OPTION) $<

include $(call rwildcard, $(dir_build), *.d)
