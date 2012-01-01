# Defines the part type that this project uses.
PART=LM3S9B90

# Program name
NAME=osc_test

# Include global toolchain settings 
include $.../toolchainsettings

#Toolchain Command Config
os:=${shell uname -s}
CC      = arm-none-eabi-gcc
LD      = arm-none-eabi-ld -v
AR      = arm-none-eabi-ar
AS      = arm-none-eabi-as
CP      = arm-none-eabi-objcopy
OD	= arm-none-eabi-objdump

CYGWIN = 0
ifneq ($(findstring CYGWIN, ${os}), )
	CYGWIN = 1
endif

ifeq ($(SERIALPROG),1)
	LINKER = bootlink.ld
	ifeq ($(WINDOWS),1)
		FLASHCMD = lmflash -q manual -i serial -p $(COMPORT) -b 115200 -r --offset=0x1000 --xfer-size=32 $(NAME).bin
	else
		FLASHCMD = sflash -p 0x1000 -c $(COMPORT) -b 115200 -s 32 $(NAME).bin
	endif
else
	LINKER = link.ld
	ifeq ($(WINDOWS),1)
		FLASHCMD = perl ./do_flash.pl $(NAME).bin
	else 
		FLASHCMD = ./do_flash.pl $(NAME).bin
	endif
endif

ifeq ($(WINDOWS),1)	
	ifeq ($(CYGWIN),1)
		CLEANCMD = rm -rf ${COMPILER} ${wildcard *~}
		COPYCMD = cp ./${COMPILER}/$(NAME).bin ./
	else
		CLEANCMD = del /s /q ${COMPILER} ${wildcard *~}
		COPYCMD = copy ./${COMPILER}/$(NAME).bin ./
	endif	
else 
	CLEANCMD = rm -rf ${COMPILER} ${wildcard *~}
	COPYCMD = cp ./${COMPILER}/$(NAME).bin ./
endif

# The base directory for StellarisWare.
ROOT=$(DIR_STELLARISWARE)

# Include the common make definitions.
include ${ROOT}/makedefs

# Where to find source files that do not live in this directory.
VPATH+=$(DIR_STELLARISWARE)/third_party/uip-1.0/apps/httpd
VPATH+=$(DIR_STELLARISWARE)/third_party/uip-1.0/uip
VPATH+=$(DIR_STELLARISWARE)/utils

# Where to find header files that do not live in the source directory.
IPATH=.
IPATH+=$(DIR_STELLARISWARE)
IPATH+=$(DIR_STELLARISWARE)/third_party/uip-1.0/apps
IPATH+=$(DIR_STELLARISWARE)/third_party/uip-1.0/uip
IPATH+=$(DIR_STELLARISWARE)/third_party/uip-1.0

# The default rule, which causes the Ethernet with uIP to be built.
all: ${COMPILER} ${COMPILER}/osc_test.axf

# The rule to clean out all the build products.
clean:	
	@echo Target 'clean' 
	rm -rf ${COMPILER} ${wildcard *~}

# Build and flash
flash: all 
	@echo Target 'flash' 
	@echo Flash command: $(FLASHCMD)
	@echo Copy command: $(COPYCMD)
	$(COPYCMD)
	$(FLASHCMD)

# The rule to create the target directory.
${COMPILER}: 
	@echo Target ${COMPILER}
	mkdir -p ${COMPILER}

# Rules for building
${COMPILER}/osc_test.axf: ${COMPILER}/osc.o
${COMPILER}/osc_test.axf: ${COMPILER}/shiftbrite.o
${COMPILER}/osc_test.axf: ${COMPILER}/dhcpc.o
${COMPILER}/osc_test.axf: ${COMPILER}/osc_test.o
${COMPILER}/osc_test.axf: ${COMPILER}/httpd.o
${COMPILER}/osc_test.axf: ${COMPILER}/startup_${COMPILER}.o
${COMPILER}/osc_test.axf: ${COMPILER}/uartstdio.o
${COMPILER}/osc_test.axf: ${COMPILER}/uip.o
${COMPILER}/osc_test.axf: ${COMPILER}/uip_arp.o
${COMPILER}/osc_test.axf: ${COMPILER}/uip_timer.o
${COMPILER}/osc_test.axf: ${COMPILER}/ustdlib.o
${COMPILER}/osc_test.axf: ${ROOT}/driverlib/${COMPILER}-cm3/libdriver-cm3.a
${COMPILER}/osc_test.axf: $(LINKER)
SCATTERgcc_osc_test=$(LINKER)
ENTRY_osc_test=ResetISR
CFLAGSgcc=-DTARGET_IS_TEMPEST_RC5
CPFLAGS=-Obinary
LFLAGS=-T $(LINKER) -nostartfiles
CP=arm-none-eabi-objcopy

#reg: $(NAME).elf
#        $(CP) $(CPFLAGS) $(NAME).elf $(NAME).bin

#$(NAME).elf: $(SOURCES) link.ld 
#        $(CC) $(MACROS) $(CFLAGS) $(LFLAGS) -o $(NAME).elf  $(SOURCES)


# Include the automatically generated dependency files.
ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif

