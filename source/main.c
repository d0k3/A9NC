#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>

#include "netloader.h"

#define NETWORK_PORT 80
#define ARM9_PAYLOAD_MAX_SIZE 0x100000

// adapted from: https://github.com/AlbertoSONIC/3DS_Quick_Reboot/blob/master/source/main.c
void quick_reboot(void) {
    //Reboot Code
    aptOpenSession();
    APT_HardwareResetAsync();
    aptCloseSession();
}

int main() {
    // Initialize services
	srvInit();
    aptInit();
    fsInit();
    sdmcInit();
    hidInit();
    acInit();
    ptmuInit();
    amInit();
    gfxInitDefault();
    gfxSet3D(false);
    consoleInit(GFX_BOTTOM, NULL);
    
    
    netloader_loop();
    // deinitialize netloader
    netloader_exit();
    
    // Deinitialize services
    gfxExit();
    amExit();
    ptmuExit();
    acExit();
    hidExit();
    sdmcExit();
    fsExit();
    aptExit();
    srvExit();
    return 0;
}
