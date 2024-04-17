DEFAULT_HOST!=../default-host.sh
HOST?=DEFAULT_HOST
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

CFLAGS?=
CPPFLAGS?=  
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

CFLAGS:=$(CFLAGS) -ffreestanding 
CPPFLAGS:=$(CPPFLAGS) -std=c++11 -fno-exceptions -fno-rtti -D__is_kernel -Iinclude 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

ARCHDIR=arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
kernel/kernel.o \
kernel/banner.o \

OBJS=\
$(ARCHDIR)/crt/crti.o \
$(ARCHDIR)/crt/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crt/crtend.o \
$(ARCHDIR)/crt/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/crt/crti.o \
$(ARCHDIR)/crt/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crt/crtend.o \
$(ARCHDIR)/crt/crtn.o \

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S

all: ParsOS.kernel

ParsOS.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot ParsOS.kernel

$(ARCHDIR)/crt/crtbegin.o $(ARCHDIR)/crt/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC)  -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.cpp.o:
	$(CC)  -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC)  -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f ParsOS.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/common
	cp -R --preserve=timestamps include/common/. $(DESTDIR)$(INCLUDEDIR)/common/.
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/kernel
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/kernel/public
	cp -R --preserve=timestamps include/kernel/public/. $(DESTDIR)$(INCLUDEDIR)/kernel/public/.


install-kernel: ParsOS.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp ParsOS.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)

