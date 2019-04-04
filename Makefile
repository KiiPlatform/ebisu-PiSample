SDK_REPO_DIR=ebisu

SDK_BUILD_DIR=build-sdk
SDK_PREFIX=usr/local
INSTALL_PATH=$(SDK_BUILD_DIR)/$(SDK_PREFIX)

CMAKE_BUILD_TYPE = RELEASE
ifdef DEBUG
CFLAGS += -g -DDEBUG
CMAKE_BUILD_TYPE = DEBUG
endif

CFLAGS += -Wall -pthread

LIBS = -lssl -lcrypto -lpthread -ltio -lwiringPi
LD_FLAGS = -L$(INSTALL_PATH)/lib
# On Mac using homebrew.
LD_FLAGS += -L/usr/local/opt/openssl/lib
SOURCES = $(wildcard *.c)
SOURCES += $(wildcard linux-env/*.c)
TARGET = exampleapp
INCLUDES = -I$(INSTALL_PATH)/include
# On Mac using homebrew.
INCLUDES += -I/usr/local/opt/openssl/include/
RPATH = -Wl,-rpath,$(INSTALL_PATH)/lib


$(SDK_REPO_DIR):
	git clone https://github.com/KiiPlatform/ebisu.git

sdk: $(SDK_REPO_DIR)
	touch $(SDK_BUILD_DIR)
	rm -r $(SDK_BUILD_DIR)
	mkdir $(SDK_BUILD_DIR)
	cd $(SDK_BUILD_DIR) && cmake $(DEBUG_OPT) ../$(SDK_REPO_DIR)/tio && make && make DESTDIR=. install

$(TARGET): sdk
	gcc $(CFLAGS) $(SOURCES) $(LIBS) $(LD_FLAGS) $(INCLUDES) -o $@

clean:
	touch $(SDK_REPO_DIR)
	rm -fr $(SDK_REPO_DIR)
	touch $(SDK_BUILD_DIR)
	rm -fr $(SDK_BUILD_DIR)
	touch $(TARGET)
	rm $(TARGET)
install-sdk:
	sudo cp $(INSTALL_PATH)/lib/* /usr/lib/; \
	sudo cp $(INSTALL_PATH)/include/* /usr/include/

deploy-service: install-sdk
	sudo cp thing-if-pi-sample.service /etc/systemd/system/; \
	sudo systemctl start thing-if-pi-sample.service; \
	sudo systemctl enable thing-if-pi-sample.service

stop-service:
	sudo systemctl stop thing-if-pi-sample.service

start-service:
	sudo systemctl start thing-if-pi-sample.service

.PHONY: sdk clean app deploy-service start-servie stop-service install-sdk
