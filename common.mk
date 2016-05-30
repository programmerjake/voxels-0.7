.PHONY: all clean distclean install bin-archive

SHELL := /bin/bash

.SUFFIXES:
.SUFFIXES: .cpp .c .o

CPPFLAGS := $(CPPFLAGS)

export CHOST
CC := $(CHOST)-gcc
export CC
CXX := $(CHOST)-g++
export CXX
CPP := $(CHOST)-cpp
export CPP
CXXCPP := $(CHOST)-cpp
export CXXCPP
AR := $(CHOST)-ar
export AR
LIBS :=
SOURCE_DIR := $(abspath .)
HEADERS := $(wildcard $(SOURCE_DIR)/include/*.h)
HEADERS += $(wildcard $(SOURCE_DIR)/include/*/*.h)
HEADERS += $(wildcard $(SOURCE_DIR)/include/*/*/*.h)
HEADERS += $(wildcard $(SOURCE_DIR)/include/*/*/*/*.h)
HEADERS += $(wildcard $(SOURCE_DIR)/include/*/*/*/*/*.h)
HEADERS += $(wildcard $(SOURCE_DIR)/include/*/*/*/*/*/*.h)
CPPFLAGS += -I$(SOURCE_DIR)/include
EXTERNAL_SOURCES_DIR := $(SOURCE_DIR)/external-lib
BUILD_DIR := $(abspath build-$(BUILD_ARCH))
OPENSSL_BUILD_DIR := $(BUILD_DIR)/openssl
OPENSSL_BUILD_SCRIPT := $(OPENSSL_BUILD_DIR)/Configure
OPENSSL_BUILD_MAKEFILE := $(OPENSSL_BUILD_DIR)/Makefile
OPENSSL_INCLUDE_DIR := $(OPENSSL_BUILD_DIR)/include
OPENSSL_LIB_DIR := $(OPENSSL_BUILD_DIR)
HEADERS += $(OPENSSL_BUILD_MAKEFILE)
LIBCRYPTO := $(OPENSSL_BUILD_DIR)/$(LIBCRYPTO_NAME)
LIBSSL := $(OPENSSL_BUILD_DIR)/$(LIBSSL_NAME)
LIBS += $(LIBSSL) $(LIBCRYPTO)
LIBPNG_BUILD_DIR := $(BUILD_DIR)/libpng
LIBPNG_BUILD_AUTOGEN := $(LIBPNG_BUILD_DIR)/autogen.sh
LIBPNG_BUILD_CONFIGURE := $(LIBPNG_BUILD_DIR)/configure
LIBPNG_BUILD_MAKEFILE := $(LIBPNG_BUILD_DIR)/Makefile
LIBPNG_INCLUDE_DIR := $(LIBPNG_BUILD_DIR)
LIBPNG_LIB_DIR := $(LIBPNG_BUILD_DIR)
HEADERS += $(LIBPNG_BUILD_MAKEFILE)
LIBPNG_LA := $(LIBPNG_BUILD_DIR)/.libs/libpng16.la
LIBPNG := $(LIBPNG_BUILD_DIR)/libpng16.a
LIBS += $(LIBPNG)
ZLIB_BUILD_DIR := $(BUILD_DIR)/zlib
ZLIB_BUILD_CONFIGURE := $(ZLIB_BUILD_DIR)/configure
ZLIB_BUILD_MAKEFILE := $(ZLIB_BUILD_DIR)/Makefile
ZLIB_INCLUDE_DIR := $(ZLIB_BUILD_DIR)
ZLIB_LIB_DIR := $(ZLIB_BUILD_DIR)
HEADERS += $(ZLIB_BUILD_MAKEFILE)
LIBZ := $(ZLIB_BUILD_DIR)/libz.a
LIBS += $(LIBZ)
VORBIS_BUILD_DIR := $(BUILD_DIR)/vorbis
VORBIS_BUILD_AUTOGEN := $(VORBIS_BUILD_DIR)/autogen.sh
VORBIS_BUILD_MAKEFILE := $(VORBIS_BUILD_DIR)/Makefile
VORBIS_INCLUDE_DIR := $(VORBIS_BUILD_DIR)/include
VORBIS_LIB_DIR := $(VORBIS_BUILD_DIR)
HEADERS += $(VORBIS_BUILD_MAKEFILE)
LIBVORBISFILE_LA := $(VORBIS_BUILD_DIR)/lib/.libs/libvorbisfile.la
LIBVORBIS := $(VORBIS_BUILD_DIR)/libvorbis.a
LIBVORBISFILE := $(VORBIS_BUILD_DIR)/libvorbisfile.a
LIBS += $(LIBVORBISFILE) $(LIBVORBIS)
OGG_BUILD_DIR := $(BUILD_DIR)/ogg
OGG_BUILD_AUTOGEN := $(OGG_BUILD_DIR)/autogen.sh
OGG_BUILD_MAKEFILE := $(OGG_BUILD_DIR)/Makefile
OGG_INCLUDE_DIR := $(OGG_BUILD_DIR)/include
OGG_LIB_DIR := $(OGG_BUILD_DIR)
HEADERS += $(OGG_BUILD_MAKEFILE)
LIBOGG_LA := $(OGG_BUILD_DIR)/src/.libs/libogg.la
LIBOGG := $(OGG_BUILD_DIR)/libogg.a
LIBS += $(LIBOGG)
SDL_BUILD_DIR := $(BUILD_DIR)/SDL
SDL_BUILD_CONFIGURE := $(SDL_BUILD_DIR)/configure
SDL_BUILD_MAKEFILE := $(SDL_BUILD_DIR)/Makefile
SDL_INCLUDE_DIR := $(SDL_BUILD_DIR)/include
SDL_LIB_DIR := $(SDL_BUILD_DIR)/build
HEADERS += $(SDL_BUILD_MAKEFILE)
LIBSDL2_LA := $(SDL_BUILD_DIR)/build/.libs/libSDL2.la
LIBSDL2 := $(SDL_BUILD_DIR)/build/libSDL2.a
LIBSDL2MAIN := $(SDL_BUILD_DIR)/build/libSDL2main.a
LIBS += $(LIBSDL2) -Wl,--whole-archive $(LIBSDL2MAIN) -Wl,--no-whole-archive
PROGRAM_NAME := voxels$(EXEEXT)
PROGRAM := $(BUILD_DIR)/$(PROGRAM_NAME)
CPP_SOURCES := $(wildcard $(SOURCE_DIR)/src/*.cpp)
CPP_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*.cpp)
CPP_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*.cpp)
CPP_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*.cpp)
CPP_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*/*.cpp)
CPP_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*/*/*.cpp)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*.c)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*.c)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*.c)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*.c)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*/*.c)
C_SOURCES += $(wildcard $(SOURCE_DIR)/src/*/*/*/*/*/*.c)
SOURCES := $(CPP_SOURCES) $(C_SOURCES)
OBJECTS := $(CPP_SOURCES:$(SOURCE_DIR)/%.cpp=$(BUILD_DIR)/%.o) $(C_SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(PROGRAM)

$(OPENSSL_BUILD_SCRIPT): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(OPENSSL_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/openssl $(OPENSSL_BUILD_DIR); } \
	&& [ -x $(OPENSSL_BUILD_SCRIPT) ]

$(OPENSSL_BUILD_MAKEFILE): $(OPENSSL_BUILD_SCRIPT)
	cd $(OPENSSL_BUILD_DIR) && ./Configure $(OPENSSL_ARCH) no-shared no-ssl2 no-ssl3 no-zlib

$(LIBSSL) : $(OPENSSL_BUILD_MAKEFILE)
	cd $(OPENSSL_BUILD_DIR) && $(MAKE)

$(LIBCRYPTO) : $(LIBSSL)

$(LIBPNG_BUILD_AUTOGEN): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(LIBPNG_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/libpng $(LIBPNG_BUILD_DIR); } \
	&& [ -x $(LIBPNG_BUILD_AUTOGEN) ]

ifeq '$(findstring mingw,$(BUILD_ARCH))' 'mingw'
$(LIBPNG_BUILD_MAKEFILE): $(LIBPNG_BUILD_AUTOGEN)
	cp -T $(LIBPNG_BUILD_DIR)/scripts/makefile.gcc $(LIBPNG_BUILD_MAKEFILE)

$(LIBPNG_BUILD_DIR)/libpng.a: $(LIBPNG_BUILD_MAKEFILE)
	cd $(LIBPNG_BUILD_DIR) && $(MAKE) CC=$(CC) AR_RC="$(AR) rcs" libpng.a

$(LIBPNG): $(LIBPNG_BUILD_DIR)/libpng.a
	cp -T $(LIBPNG_BUILD_DIR)/libpng.a $(LIBPNG) 
else
$(LIBPNG_BUILD_CONFIGURE): $(LIBPNG_BUILD_AUTOGEN)
	cd $(LIBPNG_BUILD_DIR) && ./autogen.sh

$(LIBPNG_BUILD_MAKEFILE): $(LIBPNG_BUILD_CONFIGURE) $(LIBZ)
	cd $(LIBPNG_BUILD_DIR) && CPPFLAGS="$(CPPFLAGS) -I$(ZLIB_BUILD_DIR)" LDFLAGS="$(LDFLAGS) -L$(ZLIB_BUILD_DIR)" ./configure --enable-unversioned-links --disable-dependency-tracking --disable-shared --host $(CHOST)

$(LIBPNG_LA): $(LIBPNG_BUILD_MAKEFILE)
	cd $(LIBPNG_BUILD_DIR) && $(MAKE) libpng16.la

$(LIBPNG): $(LIBPNG_LA)
	cd $(LIBPNG_BUILD_DIR) && cp -t . .libs/libpng16.a
endif

$(ZLIB_BUILD_CONFIGURE): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(ZLIB_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/zlib $(ZLIB_BUILD_DIR); } \
	&& [ -x $(ZLIB_BUILD_CONFIGURE) ]

ifeq '$(findstring mingw,$(BUILD_ARCH))' 'mingw'
$(ZLIB_BUILD_MAKEFILE): $(ZLIB_BUILD_CONFIGURE)
	cd $(ZLIB_BUILD_DIR) && cp -T win32/Makefile.gcc Makefile

$(LIBZ): $(ZLIB_BUILD_MAKEFILE)
	cd $(ZLIB_BUILD_DIR) && $(MAKE) PREFIX=$(CHOST)- libz.a
else
$(ZLIB_BUILD_MAKEFILE): $(ZLIB_BUILD_CONFIGURE)
	cd $(ZLIB_BUILD_DIR) && ./configure --static

$(LIBZ): $(ZLIB_BUILD_MAKEFILE)
	cd $(ZLIB_BUILD_DIR) && $(MAKE) libz.a
endif

$(OGG_BUILD_AUTOGEN): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(OGG_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/ogg $(OGG_BUILD_DIR); } \
	&& [ -x $(OGG_BUILD_AUTOGEN) ]

$(OGG_BUILD_MAKEFILE): $(OGG_BUILD_AUTOGEN)
	cd $(OGG_BUILD_DIR) && ./autogen.sh --disable-dependency-tracking --disable-shared --host $(CHOST)

$(LIBOGG_LA): $(OGG_BUILD_MAKEFILE)
	cd $(OGG_BUILD_DIR) && $(MAKE)

$(LIBOGG): $(LIBOGG_LA)
	cd $(OGG_BUILD_DIR) && cp -t . src/.libs/libogg.a

$(VORBIS_BUILD_AUTOGEN): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(VORBIS_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/vorbis $(VORBIS_BUILD_DIR); } \
	&& [ -x $(VORBIS_BUILD_AUTOGEN) ]

$(VORBIS_BUILD_MAKEFILE): $(VORBIS_BUILD_AUTOGEN) $(LIBOGG)
	cd $(VORBIS_BUILD_DIR) && LDFLAGS=-L$(OGG_LIB_DIR) CPPFLAGS=-I$(OGG_INCLUDE_DIR) ./autogen.sh --disable-dependency-tracking --disable-shared --host $(CHOST)

$(LIBVORBISFILE_LA): $(VORBIS_BUILD_MAKEFILE)
	cd $(VORBIS_BUILD_DIR) && $(MAKE)

$(LIBVORBISFILE): $(LIBVORBISFILE_LA)
	cd $(VORBIS_BUILD_DIR) && cp -t . lib/.libs/libvorbisfile.a

$(LIBVORBIS): $(LIBVORBISFILE_LA)
	cd $(VORBIS_BUILD_DIR) && cp -t . lib/.libs/libvorbis.a

$(SDL_BUILD_CONFIGURE): ;
	mkdir -p $(BUILD_DIR) \
	&& { rm -rf $(SDL_BUILD_DIR); cp -r -T $(EXTERNAL_SOURCES_DIR)/SDL $(SDL_BUILD_DIR); } \
	&& [ -x $(SDL_BUILD_CONFIGURE) ]

$(SDL_BUILD_MAKEFILE): $(SDL_BUILD_CONFIGURE)
	cd $(SDL_BUILD_DIR) && ./configure --disable-dependency-tracking --enable-libc --disable-sndio --disable-shared --disable-render-d3d --host $(CHOST)

$(LIBSDL2_LA): $(SDL_BUILD_MAKEFILE)
	cd $(SDL_BUILD_DIR) && $(MAKE)

$(LIBSDL2): $(LIBSDL2_LA)
	cd $(SDL_BUILD_DIR)/build && cp -t . .libs/libSDL2.a

$(LIBSDL2MAIN): $(LIBSDL2_LA)

CXXFLAGS := -std=c++11
CFLAGS += -Wall
CPPFLAGS += -I$(OPENSSL_INCLUDE_DIR)
CPPFLAGS += -I$(ZLIB_INCLUDE_DIR)
CPPFLAGS += -I$(LIBPNG_INCLUDE_DIR)
CPPFLAGS += -I$(OGG_INCLUDE_DIR)
CPPFLAGS += -I$(VORBIS_INCLUDE_DIR)
CPPFLAGS += -I$(SDL_INCLUDE_DIR)

$(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.c $(HEADERS)
	mkdir -p $(dir $@) && $(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o : $(SOURCE_DIR)/%.cpp $(HEADERS)
	mkdir -p $(dir $@) && $(CXX) $(CPPFLAGS) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

LDFLAGS += -static-libgcc -static-libstdc++

$(PROGRAM) : $(OBJECTS) $(LIBSDL2) $(LIBSDL2MAIN) $(LIBPNG) $(LIBOGG) $(LIBVORBISFILE) $(LIBVORBIS) $(LIBCRYPTO) $(LIBSSL) $(LIBZ)
	$(CXX) -o $(PROGRAM) $(OBJECTS) $(LDFLAGS) $(LIBS) $(DEFERRED_LDFLAGS)

install: all

clean:
	-rm -rf $(OBJECTS) $(PROGRAM)

distclean: clean
	-rm -rf $(BUILD_DIR)

ARCHIVE_NAME := voxels$(ARCHIVE_EXTENSION)

bin-archive: all
	mkdir -p $(BUILD_DIR)/voxels-0.7 \
	&& cp -rt $(BUILD_DIR)/voxels-0.7 $(PROGRAM) $(SOURCE_DIR)/res $(SOURCE_DIR)/LICENSE $(SOURCE_DIR)/README.md \
	&& cd $(BUILD_DIR) \
	&& { rm -f $(ARCHIVE_NAME) || true; } \
	&& $(ARCHIVE_COMMAND) $(ARCHIVE_NAME) voxels-0.7
