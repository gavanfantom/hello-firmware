# Makefile

DEVICE ?= /dev/ttyUSB0

SRCS = hello.c startup_ARMCM0.S system.c display.c font.c i2c.c speaker.c
SRCS += spi.c spiflash.c uart.c write.c zmodem.c crc.c timer.c cli.c
SRCS += lock.c fault.c button.c file.c frame.c fs.c menu.c settings.c
LITTLEFS = littlefs
LITTLEFS_SRCS = lfs.c lfs_util.c
LIBDIR = lpc/src
LIBSRCS = sysinit_11xx.c sysctl_11xx.c clock_11xx.c
PROJECT = hello
EXTENSIONS = elf bin
GCCPREFIX = arm-none-eabi-
LINKER_SCRIPT = LPC1115.ld

SRCS += $(addprefix $(LITTLEFS)/,$(LITTLEFS_SRCS))
SRCS += $(patsubst %,$(LIBDIR)/%,$(LIBSRCS))

DEBUG_OPTS = -g3 -gdwarf-2 -gstrict-dwarf
OPTS = -Os
TARGET_ARCH = -mcpu=cortex-m0 -mthumb -mfloat-abi=soft
INCLUDE = -I. -Ilpc/inc -I$(LITTLEFS)
CFLAGS = -ffunction-sections -fdata-sections -Wall -Werror $(DEBUG_OPTS) $(OPTS) $(INCLUDE) -DCORE_M0
ASFLAGS = -ffunction-sections -fdata-sections -Wall -Werror $(DEBUG_OPTS) $(OPTS) $(INCLUDE) -D__STARTUP_CLEAR_BSS -D__HEAP_SIZE=0x400
LDFLAGS = -T $(LINKER_SCRIPT) $(DEBUG_OPTS) --specs=nano.specs -Wl,--gc-sections

OBJS = $(patsubst %,%.o,$(basename $(SRCS)))
PROJECTFILES = $(foreach file,$(EXTENSIONS),$(PROJECT).$(file))

all: $(PROJECTFILES)

CC      = $(GCCPREFIX)gcc
AR      = $(GCCPREFIX)ar
OBJCOPY = $(GCCPREFIX)objcopy
OBJDUMP = $(GCCPREFIX)objdump
SIZE    = $(GCCPREFIX)size

DEPDIR := .d
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) -c $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
COMPILE.S = $(CC) -c $(DEPFLAGS) $(ASFLAGS) $(CPPFLAGS) $(TARGET_ARCH)
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@

LINK.o = $(CC) $(LDFLAGS) $(TARGET_ARCH)

$(PROJECT).elf: $(OBJS)

%.o : %.c
%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $< -o $@
	$(POSTCOMPILE)

$(LIBDIR)/%.o : $(LIBDIR)/%.c
$(LIBDIR)/%.o : $(LIBDIR)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $< -o $@
	$(POSTCOMPILE)

$(LITTLEFS)/%.o : $(LITTLEFS)/%.c
$(LITTLEFS)/%.o : $(LITTLEFS)/%.c $(DEPDIR)/%.d
	$(COMPILE.c) $< -o $@
	$(POSTCOMPILE)

%.o : %.s
%.o : %.s $(DEPDIR)/%.d
	$(COMPILE.S) $< -o $@
	$(POSTCOMPILE)

%.elf: %.o
	$(LINK.o) -Xlinker -Map=${@:.elf=.map} $^ -o $@
	$(SIZE) $@

%.bin: %.elf
	$(OBJCOPY) $< -O binary $@

%.list: %.o
	$(OBJDUMP) -d $< > $@

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

.PHONY: clean flash
clean:
	rm -f $(PROJECTFILES) $(OBJS) $(PROJECT).map

flash:
	./hello-uart.py --bootloader
	lpc21isp -control -bin $(PROJECT).bin $(DEVICE) 115200 12000
	./hello-uart.py --application

include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
