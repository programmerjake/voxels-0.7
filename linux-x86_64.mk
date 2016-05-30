BUILD_ARCH := linux-x86_64
OPENSSL_ARCH := linux-x86_64
LIBCRYPTO_BASENAME := crypto
LIBSSL_BASENAME := ssl
LIBCRYPTO_NAME := libcrypto.a
LIBSSL_NAME := libssl.a
CHOST := x86_64-linux-gnu
EXEEXT :=
ifeq '$(CFLAGS)' ''
CFLAGS := -O2 -DNDEBUG
endif
CFLAGS := $(CFLAGS)
CFLAGS += -pthread
DEFERRED_LDFLAGS := -pthread -lGL -ldl -lrt
ARCHIVE_EXTENSION := .txz
ARCHIVE_COMMAND := tar -cvaf

include common.mk