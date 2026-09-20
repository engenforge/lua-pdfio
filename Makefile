.POSIX:

# Configuration - can be overridden via command line: make LUA_VERSION=5.3
LUA_VERSION ?= 5.4
PREFIX      = /usr/local

# Standard installation paths
LIBDIR  = $(PREFIX)/lib/lua/$(LUA_VERSION)
LUADIR  = $(PREFIX)/share/lua/$(LUA_VERSION)
MANDIR  = $(PREFIX)/share/man/man3

# Flags
CFLAGS  = -Wall -Wextra -O2 -fPIC -DDEBUG -I/usr/include/lua$(LUA_VERSION)
LDFLAGS = -shared
LIBS    = -lpdfio -lz -lm

# Memory-safety tooling flags (see `memcheck` and `sanitize` targets below)
DEBUG_CFLAGS = -Wall -Wextra -g -O0 -fPIC -DDEBUG -I/usr/include/lua$(LUA_VERSION)
ASAN_CFLAGS  = $(DEBUG_CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all
ASAN_LDFLAGS = -shared -fsanitize=address,undefined
# Left blank by default: inherited from the environment if already set, and
# only needed at all if valgrind can't resolve symbols in the system's
# dynamic linker (see the `memcheck` target below for what to set it to).
DEBUGINFOD_URLS ?=

# Targets
BUILD_DIR = build
MODULE    = $(BUILD_DIR)/lua-pdfio.so
STATICLIB = $(BUILD_DIR)/liblua-pdfio.a
MANPAGE   = doc/lua-pdfio.3
SRC       = src/lua-pdfio.c src/file.c src/obj.c src/dict.c src/array.c src/stream.c
OBJ       = $(BUILD_DIR)/lua-pdfio.o $(BUILD_DIR)/file.o $(BUILD_DIR)/obj.o $(BUILD_DIR)/dict.o $(BUILD_DIR)/array.o $(BUILD_DIR)/stream.o

DEBUG_BUILD_DIR = $(BUILD_DIR)/debug
DEBUG_MODULE    = $(DEBUG_BUILD_DIR)/lua-pdfio.so
DEBUG_OBJ       = $(DEBUG_BUILD_DIR)/lua-pdfio.o $(DEBUG_BUILD_DIR)/file.o $(DEBUG_BUILD_DIR)/obj.o $(DEBUG_BUILD_DIR)/dict.o $(DEBUG_BUILD_DIR)/array.o $(DEBUG_BUILD_DIR)/stream.o

ASAN_BUILD_DIR = $(BUILD_DIR)/asan
ASAN_MODULE    = $(ASAN_BUILD_DIR)/lua-pdfio.so
ASAN_OBJ       = $(ASAN_BUILD_DIR)/lua-pdfio.o $(ASAN_BUILD_DIR)/file.o $(ASAN_BUILD_DIR)/obj.o $(ASAN_BUILD_DIR)/dict.o $(ASAN_BUILD_DIR)/array.o $(ASAN_BUILD_DIR)/stream.o

all: $(MODULE) $(MANPAGE)

$(MODULE): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

static: $(OBJ)
	$(AR) $(ARFLAGS) $(STATICLIB) $(OBJ)
	ranlib $(STATICLIB)

$(BUILD_DIR)/%.o: src/%.c src/lua-pdfio.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

$(DEBUG_MODULE): $(DEBUG_OBJ)
	$(CC) -shared -g -o $@ $(DEBUG_OBJ) $(LIBS)

$(DEBUG_BUILD_DIR)/%.o: src/%.c src/lua-pdfio.h | $(DEBUG_BUILD_DIR)
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

$(DEBUG_BUILD_DIR):
	mkdir -p $@

$(ASAN_MODULE): $(ASAN_OBJ)
	$(CC) $(ASAN_LDFLAGS) -o $@ $(ASAN_OBJ) $(LIBS)

$(ASAN_BUILD_DIR)/%.o: src/%.c src/lua-pdfio.h | $(ASAN_BUILD_DIR)
	$(CC) $(ASAN_CFLAGS) -c $< -o $@

$(ASAN_BUILD_DIR):
	mkdir -p $@

$(MANPAGE): doc/lua-pdfio.3.scdoc
	scdoc < doc/lua-pdfio.3.scdoc > $@

test-data: all
	mkdir -p test/data
	LUA_PATH="./?.lua;;" LUA_CPATH="./$(BUILD_DIR)/?.so;;" lua$(LUA_VERSION) test/generate_test_data.lua

install: $(MODULE) $(MANPAGE)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(MODULE) $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp $(MANPAGE) $(DESTDIR)$(MANDIR)

test: all test-data
	LUA_PATH="./?.lua;;" LUA_CPATH="./$(BUILD_DIR)/?.so;;" lua$(LUA_VERSION) test/test_basic.lua

# Memory-safety testing. These build separate, non-optimized copies of the
# module (debug symbols, no -DNDEBUG-style stripping) so tool output points
# at real source lines. They reuse the same Lua test suite as `make test`,
# so any leak, use-after-free, double-free, or UB hit while exercising the
# bindings fails the target.
#
# `memcheck` needs valgrind. On some systems valgrind can't resolve symbols
# in the dynamic linker unless glibc's debug symbols are available, either
# from your distro's debug package (e.g. glibc-debug on Fedora/RHEL,
# libc6-dbg on Debian/Ubuntu, debuginfod-generated on Arch) or fetched on
# demand via debuginfod. If you hit "a function redirection ... cannot be
# set up" below, set DEBUGINFOD_URLS to your distro's debuginfod server,
# e.g. DEBUGINFOD_URLS=https://debuginfod.archlinux.org make memcheck.
memcheck: $(DEBUG_MODULE) test-data
	DEBUGINFOD_URLS="$(DEBUGINFOD_URLS)" \
	LUA_PATH="./?.lua;;" LUA_CPATH="./$(DEBUG_BUILD_DIR)/?.so;;" \
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--error-exitcode=1 lua$(LUA_VERSION) test/test_basic.lua

# `sanitize` needs AddressSanitizer/UndefinedBehaviorSanitizer. The `lua`
# interpreter itself isn't sanitizer-built, so the runtime is loaded via
# LD_PRELOAD ahead of it; this is the standard way to sanitizer-instrument
# a dlopen'd C extension without rebuilding its host process.
sanitize: $(ASAN_MODULE) test-data
	LD_PRELOAD="$$($(CC) -print-file-name=libasan.so)" \
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1 \
	LUA_PATH="./?.lua;;" LUA_CPATH="./$(ASAN_BUILD_DIR)/?.so;;" \
		lua$(LUA_VERSION) test/test_basic.lua

memtest: memcheck sanitize

clean:
	rm -rf $(BUILD_DIR) test/data/*.pdf test/data/*.tmp

.PHONY: all install test clean test-data static memcheck sanitize memtest
