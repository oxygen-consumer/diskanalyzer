.PHONY: all clean install

all:
	$(MAKE) -C daemon
	$(MAKE) -C cli

clean:
	$(MAKE) -C daemon clean
	$(MAKE) -C cli clean

install:
	$(MAKE) -C daemon install
	$(MAKE) -C cli install

