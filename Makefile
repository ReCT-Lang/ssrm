## NRC Makefile

# This makefile was thrown together by khhs in the span of 10 minutes.
# Due to this, it's not the most well written thing in the world.
# But it still "works", and it does some stuff which I've never tried before.

## LICENSE

# Feel free to use it in your own projects, I do not guarantee any form
# of warranty tho.

# All I'd want you to do is to allow others to also use it, in OSS projects.
# You should also credit me(khhs)

# Just don't be an ass, okay?

## Variable descriptions

# 	BIN - The output/binary directory
# 	EXE - The binary file
# 	MODS - All the source directories(under src)
#	CPY - All ".cgen.py" files, which are python scripts that generate C code
#	HPY - All ".hgen.py" files, which are python scripts that generate C headers
# 	SRC - All the files that should be compiled
# 	HDR - All the header files in the project
#	DEPS - Auto-generated C dependencies
# 	OBJ - The object files
# 	MKDIR - The command used to create a dir(without error)
# 	RM - The command used to delete files/dirs(without errors)
# 	CP - The command used to copy files
# 	CC - The C compiler command
# 	LD - The C linker command
#	CCFLAGS - The C compiler flags
#	LDFLAGS - The C linker flags
#	LIBRARY - The C linker libraries
#	FLAGS - Various flags that may be set for compilation.

## Targets

#	$(BIN) - Makes the build directory
#	buildfiles - Copies C and header files to build dir
#	obj - Builds C files into .o
#	binary - Links the .o files into the $(EXE) file
#	all - Build everything(binary)
#	run - Builds and runs the project
#	clean - Deletes the build directory
#
#		Special targets:
#	%.c: %.cgen.py - Generates a C file from a C generator
#	%.h: %.hgen.py - Generates a H file from a header generator

EXE:=nrc

MODS:=/cli /lexer /util /parser /errors /binder

CPY:=$(foreach mod,$(MODS),$(wildcard src$(mod)/*.cgen.py))
HPY:=$(foreach mod,$(MODS),$(wildcard src$(mod)/*.hgen.py))
SRC:=$(foreach mod,$(MODS),$(wildcard src$(mod)/*.c)) $(patsubst %.cgen.py,%.c,$(CPY)) memstack/src/memstack.c
HDR:=$(foreach mod,$(MODS),$(wildcard src$(mod)/*.h)) $(patsubst %.hgen.py,%.h,$(HPY)) memstack/include/memstack.c
RCT:=$(wildcard src/r_test/*.rct)
OBJ:=$(patsubst %.c,%.o,$(SRC))

# Valid flags:
# - NRC_JOKES - Allows some non-spec features
FLAGS:=

DEPS=$(SRC:.c=.d) build.d

MKDIR:=mkdir -p
CP:=cp
RM:=rm -rf

CC:=gcc
LD:=gcc
PY:=python

CCFLAGS:=-g -Imemstack/include -Isrc
LDFLAGS:=-g
LIBRARY:=

-include $(DEPS)

$(BIN):
	$(info [MK] CREATING BUILD DIR)
	@$(MKDIR) $(BIN)

%.c: %.cgen.py
	$(info [PY] GENERATING $< => $@)
	@cd "$(dir $<)" && $(PY) $(notdir $<)

%.h: %.hgen.py
	$(info [PY] GENERATING $< => $@)
	@cd "$(dir $<)" && $(PY) $(notdir $<)

%.o: %.c
	$(info [CC] BUILDING $< => $@)
	@$(CC) $(CCFLAGS) $(foreach flag,$(FLAGS),-D$(flag)) -MMD -MP -c $< -o $@

obj: $(OBJ)

binary: obj
	$(info [LD] LINKING $(OBJ) => $(EXE))
	@$(LD) $(LDFLAGS) $(OBJ) -o $(EXE) $(LIBRARY)

all: binary $(B_RCT)

run: all
	./$(EXE)

clean:
	$(RM) $(BIN) $(OBJ) $(SRC:.c=.d)

.PHONY: clean, run, all, binary, obj