SDK_BUILD_DIR=build-sdk
SDK_REPO_DIR=thing-if-ThingSDK
SDK_PREFIX=usr/local
INSTALL_PATH=$(SDK_BUILD_DIR)/$(SDK_PREFIX)

CFLAGS += -Wall -pedantic -pthread
LIBS = -lssl -lcrypto -lpthread -lkiithingifsdk -lwiringPi
LD_FLAGS = -L$(INSTALL_PATH)/lib
# On Mac using homebrew.
LD_FLAGS += -L/usr/local/opt/openssl/lib
SOURCES = $(wildcard *.c)
SOURCES += $(wildcard linux-env/*.c)
TARGET = exampleapp
INCLUDES = -I$(INSTALL_PATH)/include
# On Mac using homebrew.
INCLUDES += -I/usr/local/opt/openssl/include/

ifdef DEBUG
	DEBUG_OPT=-DCMAKE_BUILD_TYPE=DEBUG
endif

$(SDK_REPO_DIR):
	git clone git@github.com:KiiPlatform/thing-if-ThingSDK.git
	cd $(SDK_REPO_DIR) && git submodule init && git submodule update
	cd $(SDK_REPO_DIR)/kii && git submodule init && git submodule update

sdk: $(SDK_REPO_DIR)
	touch $(SDK_BUILD_DIR)
	rm -r $(SDK_BUILD_DIR)
	mkdir $(SDK_BUILD_DIR)
	cd $(SDK_BUILD_DIR) && cmake $(DEBUG_OPT) ../$(SDK_REPO_DIR) && make && make DESTDIR=. install

$(TARGET): sdk
	gcc $(CFLAGS) $(SOURCES) $(LIBS) $(LD_FLAGS) $(INCLUDES) -o $@

clean:
	touch $(SDK_REPO_DIR)
	rm -fr $(SDK_REPO_DIR)
	touch $(SDK_BUILD_DIR)
	rm -fr $(SDK_BUILD_DIR)
	touch $(TARGET)
	rm $(TARGET)

all: clean $(TARGET)

.PHONY: sdk sdk-arm clean app app-debug app-arm app-arm-debug
