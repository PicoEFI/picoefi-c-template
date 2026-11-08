#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    (void)image_handle;

    // Print a message to the firmware's console.
    system_table->ConOut->OutputString(system_table->ConOut, L"Hello world\r\n");
    for (;;) {}
}
