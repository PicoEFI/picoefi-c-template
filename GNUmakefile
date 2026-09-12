# Nuke built-in rules.
.SUFFIXES:

# Delete the target of a failed recipe.
.DELETE_ON_ERROR:

# This is the name that our final executable will have.
# Change as needed.
override OUTPUT := efi-template

# Target architecture to build for. Default to x86_64.
ARCH := x86_64

# Install prefix; /usr/local is a good, standard default pick.
PREFIX := /usr/local

# Check if the architecture is supported.
ifeq ($(filter $(ARCH),ia32 aarch64 loongarch64 riscv64 x86_64),)
    $(error Architecture $(ARCH) not supported)
endif

# Default user QEMU flags. These are appended to the QEMU command calls.
QEMUFLAGS := -m 2G

# Internal architecture specific variables that should not be changed by the
# user. QEMU calls ia32 i386.
override QEMU_ARCH := $(subst ia32,i386,$(ARCH))
ifneq ($(filter $(ARCH),ia32 x86_64),)
    ifeq ($(ARCH),ia32)
        override EFI_BOOT_FILE := BOOTIA32.EFI
    else
        override EFI_BOOT_FILE := BOOTX64.EFI
    endif
    override QEMU_MACHINE_FLAGS := \
        -M q35
else
    ifeq ($(ARCH),aarch64)
        override QEMU_CPU := cortex-a72
        override EFI_BOOT_FILE := BOOTAA64.EFI
    endif
    ifeq ($(ARCH),riscv64)
        override QEMU_CPU := rv64
        override EFI_BOOT_FILE := BOOTRISCV64.EFI
    endif
    ifeq ($(ARCH),loongarch64)
        override QEMU_CPU := la464
        override EFI_BOOT_FILE := BOOTLOONGARCH64.EFI
    endif
    override QEMU_MACHINE_FLAGS := \
        -M virt \
        -cpu $(QEMU_CPU) \
        -device ramfb \
        -device qemu-xhci \
        -device usb-kbd \
        -device usb-tablet
endif
override QEMU_UEFI_FLAGS := \
    -drive if=pflash,unit=0,format=raw,file=edk2-ovmf-bins/ovmf-code-$(ARCH).fd,readonly=on

# User controllable toolchain and toolchain prefix.
TOOLCHAIN :=
TOOLCHAIN_PREFIX :=
ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
endif

# User controllable C compiler command.
ifneq ($(TOOLCHAIN_PREFIX),)
    CC := $(TOOLCHAIN_PREFIX)gcc
else
    CC := cc
endif

# User controllable linker command.
LD := $(TOOLCHAIN_PREFIX)ld

# User controllable objcopy command.
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

# Defaults overrides for variables if using "llvm" as toolchain.
ifeq ($(TOOLCHAIN),llvm)
    CC := clang
    LD := ld.lld
endif

# User controllable C flags.
CFLAGS := -g -O2 -pipe

# User controllable C preprocessor flags. We set none by default.
CPPFLAGS :=

ifneq ($(filter $(ARCH),ia32 x86_64),)
    # User controllable nasm flags.
    NASMFLAGS := -g
endif

# User controllable linker flags. We set none by default.
LDFLAGS :=

# Ensure the dependencies have been obtained.
ifneq ($(filter-out clean distclean,$(or $(MAKECMDGOALS),default)),)
    ifeq ($(wildcard .deps-obtained),)
        $(error Please run the ./get-deps script first)
    endif
endif

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -nostdinc \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fshort-wchar \
    -fno-lto \
    -fPIE \
    -ffunction-sections \
    -fdata-sections

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -I src \
    -I picoefi/inc \
    -isystem freestanding-c-hdrs/include \
    $(CPPFLAGS) \
    -MMD \
    -MP

ifneq ($(filter $(ARCH),ia32 x86_64),)
    # Internal nasm flags that should not be changed by the user.
    override NASMFLAGS := \
        $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
        -i src/ \
        -Wall
endif

# Make Clang target the freestanding ELF triple of the architecture. Clang
# calls ia32 i686.
ifeq ($(CC_IS_CLANG),1)
    override CC += \
        -target $(subst ia32,i686,$(ARCH))-unknown-none-elf
endif

# Architecture specific internal flags.
ifeq ($(ARCH),ia32)
    override CFLAGS += \
        -m32 \
        -march=i686 \
        -mabi=sysv \
        -mno-80387 \
        -mno-mmx \
        -malign-double
    override LDFLAGS += \
        -m elf_i386
    override NASMFLAGS := \
        -f elf32 \
        $(NASMFLAGS)
endif
ifeq ($(ARCH),x86_64)
    override CFLAGS += \
        -m64 \
        -march=x86-64 \
        -mabi=sysv \
        -mno-80387 \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -mno-red-zone
    override LDFLAGS += \
        -m elf_x86_64
    override NASMFLAGS := \
        -f elf64 \
        $(NASMFLAGS)
endif
ifeq ($(ARCH),aarch64)
    override CFLAGS += \
        -mcpu=generic \
        -march=armv8-a+nofp+nosimd \
        -mno-outline-atomics \
        -mgeneral-regs-only
    override LDFLAGS += \
        -m aarch64elf
endif
ifeq ($(ARCH),riscv64)
    override CFLAGS += \
        -march=rv64imac_zicsr_zifencei \
        -mabi=lp64 \
        -mno-relax
    override LDFLAGS += \
        -m elf64lriscv \
        --no-relax
endif
ifeq ($(ARCH),loongarch64)
    override CFLAGS += \
        -march=loongarch64 \
        -mabi=lp64s \
        -mfpu=none \
        -msimd=none \
        -mno-relax
    override LDFLAGS += \
        -m elf64loongarch \
        --no-relax
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
    -nostdlib \
    -pie \
    -z text \
    -z max-page-size=0x1000 \
    -z noexecstack \
    --gc-sections \
    -T picoefi/$(ARCH)/link_script.lds

# Use "find" to glob all *.c, *.S, and *.asm files in the tree
# (except the src/arch/* directories, as those are gonna be added
# in the next step).
override SRCFILES := $(shell find -L src cc-runtime/src picoefi/$(ARCH) -type f ! -path 'src/arch/*' 2>/dev/null | LC_ALL=C sort)
# Add architecture specific files, if they exist.
override SRCFILES += $(shell find -L src/arch/$(ARCH) -type f 2>/dev/null | LC_ALL=C sort)
# Obtain the object and header dependencies file names.
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
override OBJ := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))
override HEADER_DEPS := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))
ifneq ($(filter $(ARCH),ia32 x86_64),)
override NASMFILES := $(filter %.asm,$(SRCFILES))
override OBJ += $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.o))
override HEADER_DEPS += $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.d))
endif

# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin-$(ARCH)/$(OUTPUT).efi

# Include header dependencies.
-include $(HEADER_DEPS)

# Rule to convert the final ELF executable to a .EFI PE executable.
bin-$(ARCH)/$(OUTPUT).efi: bin-$(ARCH)/$(OUTPUT) GNUmakefile
	mkdir -p "$(dir $@)"
	$(OBJCOPY) -O binary $< $@
	dd if=/dev/zero of=$@ bs=4096 count=0 seek=$$(( ($$(wc -c < $@) + 4095) / 4096 )) 2>/dev/null

# Link rules for the final executable.
bin-$(ARCH)/$(OUTPUT): GNUmakefile picoefi/$(ARCH)/link_script.lds $(OBJ)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) -o $@

# Compilation rules for *.c files.
obj-$(ARCH)/%.c.o: %.c GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj-$(ARCH)/%.S.o: %.S GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

ifneq ($(filter $(ARCH),ia32 x86_64),)
# Compilation rules for *.asm (nasm) files.
obj-$(ARCH)/%.asm.o: %.asm GNUmakefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) -MD $(@:.o=.d) -MP $< -o $@
endif

# Rules to download the UEFI firmware per architecture for testing.
.INTERMEDIATE: edk2-ovmf-bins.tar.gz
edk2-ovmf-bins.tar.gz:
	curl -fL -o $@ https://github.com/osdev0/edk2-ovmf-stable-bins/releases/latest/download/edk2-ovmf-bins.tar.gz

edk2-ovmf-bins: edk2-ovmf-bins.tar.gz
	rm -rf edk2-ovmf-bins
	gunzip < edk2-ovmf-bins.tar.gz | tar -xf -

# Rules for running our executable in QEMU.
.PHONY: run
run: all edk2-ovmf-bins
	mkdir -p boot/EFI/BOOT
	cp bin-$(ARCH)/$(OUTPUT).efi boot/EFI/BOOT/$(EFI_BOOT_FILE)
	qemu-system-$(QEMU_ARCH) \
		$(QEMU_MACHINE_FLAGS) \
		$(QEMU_UEFI_FLAGS) \
		-drive file=fat:rw:boot \
		$(QEMUFLAGS)
	rm -rf boot

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin-$(ARCH) obj-$(ARCH)

# Remove everything built and generated including downloaded dependencies.
.PHONY: distclean
distclean:
	rm -rf bin-* obj-* .deps-obtained .cache compile_commands.json freestanding-c-hdrs cc-runtime picoefi edk2-ovmf-bins edk2-ovmf-bins.tar.gz

# Install the final built executable to its final on-root location.
.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
	install -m 644 bin-$(ARCH)/$(OUTPUT).efi "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH).efi"

# Try to undo whatever the "install" target did.
.PHONY: uninstall
uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH).efi"
	-rmdir "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
