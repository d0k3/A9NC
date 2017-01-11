// Base code from FakeBrick9 by Wolfvak

#include "common.h"
#include "loader.h"

#include "i2c.h"

static FATFS fs;


void halt()
{
    while(!HID_PAD); // Wait for a keypress

    i2cWriteRegister(0x03, 0x20, 1); // Shutdown (device 3, register 0x20)
    while(1);
}

int main()
{
    uint8_t *payload_dest = (uint8_t*)0x24F00000, *loader_dest = (uint8_t*)0x25F00000;

    if (f_mount(&fs, "0:", 0) != FR_OK) halt(); // Mount SD card

    FIL payload;
    FRESULT f_ret;
    size_t payload_size;
	
    f_ret = f_open(&payload, PATH_TEMP, FA_READ); // Attempt to open the received payload
	if (f_ret != FR_OK) f_ret = f_open(&payload, PATH_PAYLOAD, FA_READ); // Use the regular one as a fallback
	
    if (f_ret == FR_OK) f_ret = f_read(&payload, payload_dest, f_size(&payload), &payload_size);
    else halt();

    f_close(&payload);

	f_unlink(PATH_TEMP); // Remove the received payload

    f_mount(NULL, "0:", 0); // Unmount the SD card

	memcpy((uint8_t*)loader_dest, loader_bin, loader_bin_len);
	
	((void(*)(void))(uint32_t)loader_dest)();
    return 0;
}
