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

# User controllable objcopy command.
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

# Defaults overrides for variables if using "llvm" as toolchain.
ifeq ($(TOOLCHAIN),llvm)
    CC := clang
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

# Check if CFLAGS enables LTO, decided by the last -flto or -fno-lto.
override CFLAGS_HAS_LTO := $(shell ! printf '%s\n' $(CFLAGS) | grep -E '^-f(no-)?lto(=|$$)' | tail -n 1 | grep -q '^-flto'; echo $$?)

# Macros to check if a flag is supported, used as $(call MACRO,flag), and
# expanding to 1 or 0. A second argument names another spelling.

# The name of a flag, up to any "=", with the dashes after the first made
# optional, as compilers leave the value out or spell the flag differently.
override FLAG_NAME = -$(subst -,-?,$(patsubst -%,%,$(firstword $(subst =, ,$(1)))))

# Look for a diagnostic naming the flag, in the untranslated C locale. The
# name has to stand on its own, as "-pie" would otherwise be found inside
# a complaint about "-no-pie".
override FLAG_IS_NAMED = grep -qE '(error|warning|note):.*[^[:alnum:]]($(call FLAG_NAME,$(1))$(if $(2),|$(call FLAG_NAME,$(2))))([^-[:alnum:]]|$$)'

# Check if the compiler supports a flag when compiling.
override CC_HAS_COMPILE_FLAG = $(shell LC_ALL=C $(CC) $(CFLAGS) $(1) -c -x c /dev/null -o /dev/null 2>&1 >/dev/null | $(call FLAG_IS_NAMED,$(1),$(2)); echo $$?)

# Check if the compiler supports a flag when linking, linking nothing.
override CC_HAS_LINK_FLAG = $(shell LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) $(1) -Wl,--version 2>&1 >/dev/null | $(call FLAG_IS_NAMED,$(1),$(2)); echo $$?)

# Check if the linker supports a flag by searching its help, as older GNU
# ld take unknown flags for different ones (-no-pie for -n -o -pie).
override LD_HAS_FLAG = $(shell ! $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--help 2>/dev/null | grep -qE '(^|[[:space:]])-?$(1)([[:space:],=[]|$$)'; echo $$?)

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-common \
    -fno-stack-protector \
    -fno-stack-check \
    -fshort-wchar \
    -fPIE \
    -ffunction-sections \
    -fdata-sections

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -nostdinc \
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
    # Use LLD unless LDFLAGS picks another linker.
    override LDFLAGS := \
        -fuse-ld=lld \
        $(LDFLAGS)
endif

# Check that the compiler works with the given flags, as the checks below
# would otherwise blame a missing feature.
override CC_COMPILE_ERROR := $(shell LC_ALL=C $(CC) $(CFLAGS) -c -x c /dev/null -o /dev/null 2>&1 >/dev/null | grep -i 'error:' | head -n 1)
ifneq ($(CC_COMPILE_ERROR),)
    $(error The compiler does not work with the given CFLAGS: $(CC_COMPILE_ERROR))
endif

ifeq ($(call CC_HAS_COMPILE_FLAG,-fno-stack-clash-protection),1)
    override CFLAGS += \
        -fno-stack-clash-protection
endif

ifeq ($(CC_IS_CLANG),1)
    # Clang hands freestanding links over to another compiler driver on
    # some targets. Look for the linker flags being passed on as they
    # are, which a linker Clang runs itself would be given translated,
    # as the driver it picks need not be named "gcc" at all. The check
    # is made again below, so it runs on each use. The print flag is
    # kept in a variable, as older makes take a "#" in a call for a
    # comment.
    override CLANG_PRINT_COMMANDS := -\#\#\#
    override CC_LINKS_VIA_DRIVER = $(shell ! $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--version $(CLANG_PRINT_COMMANDS) 2>&1 | grep '^ "' | tail -n 1 | grep -q -- '-Wl,'; echo $$?)

    # If it does, link as for the Linux target, where Clang runs the
    # linker itself, and keep that target's configuration files out. The
    # flag is looked for by compiling, as linking is not checked yet.
    ifeq ($(CC_LINKS_VIA_DRIVER),1)
        override LDFLAGS += \
            -target $(subst ia32,i686,$(ARCH))-linux-gnu
        ifeq ($(call CC_HAS_COMPILE_FLAG,--no-default-config),1)
            override LDFLAGS += \
                --no-default-config
        endif

        # Check that this worked, as the linker flags would otherwise
        # reach a compiler driver that knows nothing of them.
        ifeq ($(CC_LINKS_VIA_DRIVER),1)
            $(error The compiler still hands the link over to another compiler driver)
        endif
    endif
endif

# The same for linking, once Clang has a linker it can run.
override CC_LINK_ERROR := $(shell LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--version 2>&1 >/dev/null | grep -i 'error:' | head -n 1)
ifneq ($(CC_LINK_ERROR),)
    $(error The linker does not work with the given LDFLAGS: $(CC_LINK_ERROR))
endif

# Architecture specific internal flags.
ifeq ($(ARCH),ia32)
    override CFLAGS += \
        -m32 \
        -march=i686 \
        -mno-mmx \
        -malign-double
    # GCC needs this flag for interrupt handlers and Clang below 3.9 has
    # no name for it, where only a long double could reach the x87.
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-80387),1)
        override CFLAGS += \
            -mno-80387
    endif
    # Clang below 9 takes -malign-double and ignores it, which lays out
    # every firmware structure with a 64 bit member wrong, so check the
    # alignment rather than the flag.
    override CC_ALIGNS_64BIT := $(shell ! printf 'struct s { char c; long long x; };\n_Static_assert(sizeof(struct s) == 16, "");\n' | LC_ALL=C $(CC) $(CFLAGS) -c -x c - -o /dev/null 2>/dev/null; echo $$?)
    ifneq ($(CC_ALIGNS_64BIT),1)
        $(error The compiler does not align 64 bit types to 8 bytes)
    endif
    ifeq ($(call CC_HAS_COMPILE_FLAG,-fcf-protection=none),1)
        override CFLAGS += \
            -fcf-protection=none
    endif
    override LDFLAGS += \
        -Wl,-m,elf_i386
    override NASMFLAGS := \
        -f elf32 \
        $(NASMFLAGS)
endif
ifeq ($(ARCH),x86_64)
    override CFLAGS += \
        -m64 \
        -march=x86-64 \
        -mno-mmx \
        -mno-sse \
        -mno-red-zone
    # GCC needs this flag for interrupt handlers and Clang below 3.9 has
    # no name for it, where only a long double could reach the x87.
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-80387),1)
        override CFLAGS += \
            -mno-80387
    endif
    ifeq ($(call CC_HAS_COMPILE_FLAG,-fcf-protection=none),1)
        override CFLAGS += \
            -fcf-protection=none
    endif
    override LDFLAGS += \
        -Wl,-m,elf_x86_64
    override NASMFLAGS := \
        -f elf64 \
        $(NASMFLAGS)
endif
ifeq ($(ARCH),aarch64)
    override CFLAGS += \
        -mcpu=generic \
        -march=armv8-a+nofp+nosimd
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-outline-atomics),1)
        override CFLAGS += \
            -mno-outline-atomics
    endif
    override CFLAGS += \
        -mcmodel=small
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mbranch-protection=none),1)
        override CFLAGS += \
            -mbranch-protection=none
    endif
    override LDFLAGS += \
        -Wl,-m,aarch64elf
    # Only the linker can work around Cortex-A53 erratum 843419, so
    # require one that can.
    ifeq ($(call LD_HAS_FLAG,--fix-cortex-a53-843419),1)
        override LDFLAGS += \
            -Wl,--fix-cortex-a53-843419
    else
        $(error The linker cannot work around Cortex-A53 erratum 843419)
    endif
endif
ifeq ($(ARCH),riscv64)
    # The ABI comes first, as the instruction set is checked against it.
    override CFLAGS += \
        -mabi=lp64
    # Name Zicsr and Zifencei only if the compiler knows them, as older
    # ones have them in the base ISA. The whole compile is checked, as the
    # flag is not always named when an instruction set is rejected.
    override CC_HAS_ZICSR_ZIFENCEI := $(shell ! $(CC) $(CFLAGS) -march=rv64imac_zicsr_zifencei -c -x c /dev/null -o /dev/null 2>/dev/null; echo $$?)
    ifeq ($(CC_HAS_ZICSR_ZIFENCEI),1)
        override CFLAGS += \
            -march=rv64imac_zicsr_zifencei
    else
        override CFLAGS += \
            -march=rv64imac
    endif
    # Clang called this code model "small" before the ISA's own name.
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mcmodel=medlow,-mcode-model),1)
        override CFLAGS += \
            -mcmodel=medlow
    else
        ifeq ($(call CC_HAS_COMPILE_FLAG,-mcmodel=small,-mcode-model),1)
            override CFLAGS += \
                -mcmodel=small
        else
            $(error The compiler has no name for the low code model)
        endif
    endif
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-relax),1)
        override CFLAGS += \
            -mno-relax
    endif
    ifeq ($(call CC_HAS_COMPILE_FLAG,-fcf-protection=none),1)
        override CFLAGS += \
            -fcf-protection=none
    endif
    override LDFLAGS += \
        -Wl,-m,elf64lriscv \
        -Wl,--no-relax
endif
ifeq ($(ARCH),loongarch64)
    # -msoft-float, unlike -mfpu=none, also overrides a -mdouble-float or
    # -msingle-float in CFLAGS, which take effect wherever they appear.
    override CFLAGS += \
        -mabi=lp64s \
        -march=loongarch64 \
        -msoft-float
    # Clang 16 takes the soft float flags but still records the double
    # float ABI, which no linker mixes with soft float objects, so read
    # the ABI out of the ELF header it writes. LTO is turned off for the
    # check, as it would write bitcode with no such header instead.
    ifeq ($(CC_IS_CLANG),1)
        override CC_FLOAT_ABI := $(shell LC_ALL=C $(CC) $(CFLAGS) -fno-lto -c -x c /dev/null -o - 2>/dev/null | od -A n -t x1 -j 48 -N 1)
        ifeq ($(filter %1,$(CC_FLOAT_ABI)),)
            $(error The compiler does not record the soft float ABI)
        endif
    endif
    # Clang called this code model "small" before the ISA's own name.
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mcmodel=normal,-mcode-model),1)
        override CFLAGS += \
            -mcmodel=normal
    else
        ifeq ($(call CC_HAS_COMPILE_FLAG,-mcmodel=small,-mcode-model),1)
            override CFLAGS += \
                -mcmodel=small
        else
            $(error The compiler has no name for the normal code model)
        endif
    endif
    ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-relax),1)
        override CFLAGS += \
            -mno-relax
    endif
    # Do not go through the GOT for external symbols. Only GCC is checked,
    # as Clang had its flag before the architecture existed.
    ifeq ($(CC_IS_CLANG),1)
        override CFLAGS += \
            -fdirect-access-external-data
    else
        ifeq ($(call CC_HAS_COMPILE_FLAG,-mdirect-extern-access),1)
            override CFLAGS += \
                -mdirect-extern-access
        endif
    endif
    # Some Clangs do not record the LoongArch ABI in LTO objects, so pass
    # it to LTO if the IR of an empty file does not record it.
    ifeq ($(CC_IS_CLANG),1)
        ifeq ($(CFLAGS_HAS_LTO),1)
            override CC_LTO_HAS_ABI := $(shell ! $(CC) $(CFLAGS) -S -emit-llvm -x c /dev/null -o - 2>/dev/null | grep -q target-abi; echo $$?)

            ifneq ($(CC_LTO_HAS_ABI),1)
                override LDFLAGS += \
                    -Wl,-plugin-opt=-target-abi=lp64s
            endif
        endif
    endif
    override LDFLAGS += \
        -Wl,-m,elf64loongarch \
        -Wl,--no-relax
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
    -nostdlib \
    -Wl,-pie \
    -Wl,-z,text \
    -Wl,-z,max-page-size=0x1000 \
    -Wl,-z,noexecstack \
    -Wl,--gc-sections \
    -Wl,--build-id=none \
    -Wl,--hash-style=gnu \
    -Wl,-T,picoefi/$(ARCH)/link_script.lds

# Tell the compiler as well if it takes the flag, as it otherwise passes
# its own default on to the linker.
ifeq ($(call CC_HAS_LINK_FLAG,-pie),1)
    override LDFLAGS += \
        -pie
endif

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
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

# The compiler may emit calls to the memory functions and the compiler
# runtime that LTO does not account for, so never build those with LTO.
obj-$(ARCH)/src/memory.c.o obj-$(ARCH)/cc-runtime/src/cc-runtime.c.o: override CFLAGS += -fno-lto

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
