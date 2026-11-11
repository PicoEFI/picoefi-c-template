#include <stdbool.h>
#include <stddef.h>
#include <efi.h>

efi_status efi_main(efi_handle image_handle, struct efi_system_table *system_table) {
    (void)image_handle;

    // Print a message and a prompt to the firmware's console, after a newline,
    // as some firmware does not end its own messages with one.
    system_table->con_out->output_string(
        system_table->con_out,
        L"\r\n"
        L"Hello world\r\n"
        L"Press any key to return to firmware..."
    );

    // Disable the 5-minute watchdog timer that the boot manager arms before
    // starting the program, which would reset the system during a long wait.
    system_table->boot_services->set_watchdog_timer(0, 0, 0, NULL);

    // Wait for a key press, ignoring any made before the prompt, then return
    // to the firmware.
    system_table->con_in->reset(system_table->con_in, false);
    struct efi_input_key key;
    size_t index;
    while (system_table->con_in->read_key_stroke(system_table->con_in, &key) == EFI_NOT_READY) {
        system_table->boot_services->wait_for_event(
            1, &system_table->con_in->wait_for_key, &index
        );
    }

    return EFI_SUCCESS;
}
