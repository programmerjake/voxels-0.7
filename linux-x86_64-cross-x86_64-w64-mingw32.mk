BUILD_ARCH := cross-x86_64-w64-mingw32
OPENSSL_ARCH := mingw64
LIBCRYPTO_BASENAME := crypto
LIBSSL_BASENAME := ssl
LIBCRYPTO_NAME := libcrypto.a
LIBSSL_NAME := libssl.a
CHOST := x86_64-w64-mingw32
EXEEXT := .exe
ifeq '$(CFLAGS)' ''
CFLAGS := -O2 -DNDEBUG
endif
CFLAGS := $(CFLAGS)
CFLAGS += -pthread -mwin32 -mwindows
DEFERRED_LDFLAGS := -static -pthread -lopengl32 -mwin32 -mwindows -lws2_32 -lgdi32 -lcrypt32 -lole32
DEFERRED_LDFLAGS += -limm32 -lversion -loleaut32 -lwinmm
ARCHIVE_EXTENSION := .zip
ARCHIVE_COMMAND := zip -r

include common.mk
