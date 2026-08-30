#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/gamecube_rules

ifeq ($(REGION),)

default:
	@$(MAKE) --no-print-directory REGION=us 

clean:
	@$(MAKE) --no-print-directory clean_target REGION=us 

#---------------------------------------------------------------------------------
# For now, make elf2rel a phony target
# Place target here (instead of inside recursive Makefile call) so it's only built once
#---------------------------------------------------------------------------------

# Unexport some compiler vars exported by devkitppc as they interfere
# when we build elf2rel, which uses the system compiler
unexport AS
unexport CC
unexport CXX
unexport AR
unexport OBJCOPY
unexport STRIP
unexport NM
unexport RANLIB

else

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	mkb2.rel_sample
BUILD		:=	build
SOURCES		:=	src $(wildcard src/*) 3rdparty
DATA		:=	data  
INCLUDES	:=	src 3rdparty

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

MACHDEP		= -mno-sdata -mgcn -DGEKKO -mcpu=750 -meabi -mhard-float

# -Wno-write-strings because some GC SDK functions take non-const char *,
# and Ghidra can't represent const char * anyhow
CFLAGS		= -lgcc -ffunction-sections -fdata-sections -g -Os \
			  -Wall -Werror -Wshadow -Wimplicit-fallthrough \
			  -Wno-write-strings -Wno-address-of-packed-member \
			  $(MACHDEP) $(INCLUDE)
CXXFLAGS	= -nostdlib -fno-exceptions -fno-rtti -std=c++20 $(CFLAGS)
ASFLAGS     = -mregnames # Don't require % in front of register names

LDFLAGS		= -r -e _prolog -u _prolog -u _epilog -u _unresolved -Wl,--gc-sections \
			  -nostdlib -lgcc -g $(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
# Temporarily remove the math library to avoid conflicts
# LIBS	:= -lm
LIBS	:= 

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS     :=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

# For REL linking
export LDFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ld)))
export MAPFILE		:= $(CURDIR)/src/mkb/mkb2.$(REGION).lst

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)) \
			-I$(CURDIR)/$(BUILD) \
			-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
			-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean_target

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean_target:
	@echo clean ... default
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol $(OUTPUT).rel 

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

ELF2REL := $(abspath $(CURDIR))/../3rdparty/elf2rel/build/elf2rel

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).rel: $(OUTPUT).elf $(MAPFILE) elf2rel
$(OUTPUT).elf: $(LDFILES) $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

# REL linking
%.rel: %.elf
	@echo elf2rel ... $(notdir $@)
	@$(ELF2REL) $< $(MAPFILE) $@ 100 2

#---------------------------------------------------------------------------------
# build tools
#---------------------------------------------------------------------------------

HOST_CXX ?= c++
ELF2REL_SRC := $(CURDIR)/../3rdparty/elf2rel
ELF2REL := $(CURDIR)/elf2rel
ELF2REL_HEADERS := $(ELF2REL_SRC)/elf2rel.h $(wildcard $(ELF2REL_SRC)/elfio/*.hpp)

elf2rel: $(ELF2REL)

$(ELF2REL): $(ELF2REL_SRC)/elf2rel.cpp $(ELF2REL_HEADERS)
	@echo "Compiling elf2rel..."
	@mkdir -p $(CURDIR)/build
	@$(HOST_CXX) -std=c++20 -O2 -I$(ELF2REL_SRC) $< -o $@
	
#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	%_jpg.h :	%.jpg
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
endif
