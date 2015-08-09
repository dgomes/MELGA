
OS := $(shell uname)
ifeq ($(OS),  Darwin)
# Run MacOS commands
CFLAGS+=-DOSX
endif

