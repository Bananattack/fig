ifeq ($(OS),Windows_NT)
	EXE := .exe
	LOCATE_COMMAND := where
else
	EXE :=
	LOCATE_COMMAND := command -v
endif

HAS_GCC := $(shell $(LOCATE_COMMAND) gcc)
HAS_CLANG := $(shell $(LOCATE_COMMAND) clang)
HAS_PYTHON := $(shell $(LOCATE_COMMAND) python)

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
FIG_GIF2GIF := fig_gif2gif$(EXE)
FIG_FIREBALL := fig_fireball$(EXE)

.PHONY: clean all

define uniq
	$(eval seen :=)
	$(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
	${seen}
endef
DIRECTORIES := $(call uniq, $(dir \
	$(FIG_O) \
	))

all: $(DIRECTORIES) $(FIG_GIF2PPM) $(FIG_GIF2GIF) $(FIG_FIREBALL) $(FIG_LIB)

$(FIG_O): obj/%.o: %.c $(FIG_H)
	$(CC) $(CFLAGS) -MMD -c -o $@ $< $(INCLUDES)

$(FIG_LIB): $(FIG_O)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@
ifdef HAS_PYTHON
	python ./build_single_header.py
endif	

$(FIG_GIF2PPM): tests/fig_gif2ppm.c $(FIG_LIB)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ $(INCLUDES)

$(FIG_GIF2GIF): tests/fig_gif2gif.c $(FIG_LIB)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ $(INCLUDES)

$(FIG_FIREBALL): tests/fig_fireball.c $(FIG_LIB)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@ $(INCLUDES)

clean:
	rm -rf obj $(FIG_GIF2GIF)

define makedir
$(1):
	@mkdir -p $(1)
endef
$(foreach d, $(DIRECTORIES), $(eval $(call makedir, $(d))))	

-include $(FIG_DEPS)