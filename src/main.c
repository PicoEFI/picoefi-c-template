#include <stddef.h>
#include <efi.h>

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    (void)image_handle;

    // Print a message and a prompt to the firmware's console, after a newline,
    // as some firmware does not end its own messages with one.
    system_table->ConOut->OutputString(
        system_table->ConOut,
        L"\r\n"
        L"Hello world\r\n"
        L"Press any key to return to firmware..."
    );

    // Disable the 5-minute watchdog timer that the boot manager arms before
    // starting the program, which would reset the system during a long wait.
    system_table->BootServices->SetWatchdogTimer(0, 0, 0, NULL);

    // Wait for a key press, ignoring any made before the prompt, then return
    // to the firmware.
    system_table->ConIn->Reset(system_table->ConIn, EFI_FALSE);
    EFI_INPUT_KEY key;
    EFI_UINTN index;
    while (system_table->ConIn->ReadKeyStroke(system_table->ConIn, &key) == EFI_NOT_READY) {
        system_table->BootServices->WaitForEvent(1, &system_table->ConIn->WaitForKey, &index);
    }

    return EFI_SUCCESS;
}
