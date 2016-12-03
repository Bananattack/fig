ifeq ($(OS),Windows_NT)
	EXE := .exe
	LOCATE_COMMAND := where
else
	EXE :=
	LOCATE_COMMAND := command -v
endif

HAS_GCC := $(shell $(LOCATE_COMMAND) gcc)
HAS_CLANG := $(shell $(LOCATE_COMMAND) clang)

ifdef HAS_GCC
	CC := gcc
else ifdef HAS_CLANG
	CC := clang
endif

FIG_H := $(wildcard include/*.h)
FIG_C := $(wildcard src/*.c)
FIG_O := $(patsubst %.c, obj/%.o, $(FIG_C))
FIG_LIB := obj/libfig.a
FIG_DEPS := $(sort $(patsubst %.o, %.d, $(FIG_O)))

CFLAGS := -O2 -ansi -pedantic -Wall -Werror -Lobj
LDFLAGS := -lm -lfig
INCLUDES := -Iinclude

AR := ar
ARFLAGS := cr
RANLIB := ranlib

FIG_GIF2PPM := fig_gif2ppm$(EXE)

.PHONY: clean all

define uniq
	$(eval seen :=)
	$(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
	${seen}
endef
DIRECTORIES := $(call uniq, $(dir \
	$(FIG_O) \
	))

all: $(DIRECTORIES) $(FIG_GIF2PPM) $(FIG_LIB)

$(FIG_O): obj/%.o: %.c $(FIG_H)
	$(CC) $(CFLAGS) -MMD -c -o $@ $< $(INCLUDES)

$(FIG_LIB): $(FIG_O)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@	

$(FIG_GIF2PPM): tests/fig_gif2ppm.c $(FIG_LIB)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ $(INCLUDES)

clean:
	rm -rf obj $(FIG_GIF2PPM)

define makedir
$(1):
	@mkdir -p $(1)
endef
$(foreach d, $(DIRECTORIES), $(eval $(call makedir, $(d))))	

-include $(FIG_DEPS)