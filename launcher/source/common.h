#pragma once

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "fatfs/ff.h"

#define HID_PAD         (*(volatile uint32_t*)0x10146000 ^ 0xFFF) // HID_PAD

#define PATH_TEMP       "/A9NC/temp.bin"
#define PATH_PAYLOAD    "/A9NC/payload.bin"

void halt();