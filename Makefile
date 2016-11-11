all: static

ARCH:=$(shell uname -m)
BIT32:=i686
BIT64:=x86_64

static:
	make -C ikcp 
	make -C NetFilter/src/
	make -C Net/src/
	make -C demo/KCP_Server/
	make -C demo/KCP_Client/

clean:
	make -C ikcp clean
	make -C NetFilter/src clean
	make -C Net/src clean
	make -C demo/KCP_Server/ clean
	make -C demo/KCP_Client/ clean
