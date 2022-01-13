ifdef X64
	MINGW1=$(shell which amd64-mingw32msvc-cc 2> /dev/null)
	MINGW2=$(shell which x86_64-w64-mingw32-gcc 2> /dev/null)
else
	MINGW1=$(shell which i586-mingw32msvc-cc 2> /dev/null)
	MINGW2=$(shell which i686-w64-mingw32-gcc 2> /dev/null)
endif
ifneq ("$(MINGW1)","")
	EXE_CC=$(MINGW1)
else ifneq ("$(MINGW2)","")
	EXE_CC=$(MINGW2)
else
	EXE_CC=$(error MinGW not found)
endif
RM			= rm -f

.PHONY : all
all : hello hello.exe

.PHONY : clean
clean :
	$(RM) hello hello.exe

hello : hello.c ../common/call_vmm.c ../common/call_vmm.h
	$(CC) -s -o hello hello.c ../common/call_vmm.c

hello.exe : hello.c ../common/call_vmm.c ../common/call_vmm.h
	$(EXE_CC) -s -o hello.exe hello.c ../common/call_vmm.c
