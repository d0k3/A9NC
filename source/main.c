#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/_default_fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sochlp.h"
#include "hid.h"

#define PAYLOAD_PATH "/arm9testpayload.bin"
#define NETWORK_PORT 80
#define ARM9_PAYLOAD_MAX_SIZE 0x80000

// adapted from: https://github.com/patois/Brahma/blob/master/source/brahma.c#L168-L259
s32 recv_arm9_payload (void) {// careful here!!!
	s32 arm9payload_size = 0;
    u8* arm9payload_buf = NULL;
    
	s32 sockfd;
	s32 clientfd;
	struct sockaddr_in sa;
	struct sockaddr_in client_addr;
	socklen_t addrlen = sizeof(client_addr);
	s32 sflags = 0;
    
    // init socket
    soc_init();

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("[!] Error: socket()\n");
		return 0;
	}

	memset(&sa, 0x00, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(NETWORK_PORT);
	sa.sin_addr.s_addr = gethostid();

	if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
		printf("[!] Error: bind()\n");
		close(sockfd);
		return 0;
	}

	if (listen(sockfd, 1) != 0) {
		printf("[!] Error: listen()\n");
		close(sockfd);
		return 0;
	}

	printf("[x] IP %s:%d\n", inet_ntoa(sa.sin_addr), NETWORK_PORT);

	sflags = fcntl(sockfd, F_GETFL);
	if (sflags == -1) {
		printf("[!] Error: fcntl() (1)\n");
		close(sockfd);
	}
	fcntl(sockfd, F_SETFL, sflags | O_NONBLOCK);

	hidScanInput();
    u32 old_kDown = hidKeysDown();
	do {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown != old_kDown) {
			printf("[!] Aborted\n");
			close(sockfd);
			return 0;
		}

		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		svcSleepThread(100000000);
		if (clientfd > 0)
			break;
	}

	printf("[x] Connection from %s:%d\n\n", inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port));

	s32 recvd;
	u32 total = 0;
    s32 failcount = 0;
    arm9payload_buf = (u8*) malloc(ARM9_PAYLOAD_MAX_SIZE);
    if (!arm9payload_buf) {
        printf("[!] Error: out of memory\n");
        return 0;
    }
	while (aptMainLoop() && ((recvd = recv(clientfd, arm9payload_buf + total,
      ARM9_PAYLOAD_MAX_SIZE - total, 0)) != 0)) {
		if (recvd != -1) {
			total += recvd;
			printf(".");
            failcount = 0;
		} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            total = 0;
			printf("[!] Error: netloader error\n");
            break;
        } else failcount++;
		if (total >= ARM9_PAYLOAD_MAX_SIZE) {
			total = 0;
			printf("[!] Error: invalid payload size\n");
			break;
		}
        if (failcount >= 0x1000) {
            if (total > 0) {
                printf(" [!] timeout, assumed complete");
            } else {
                printf(" [!] timeout, failed!");
                total = 0;
            }
            break;
        }
	} while (aptMainLoop());

	fcntl(sockfd, F_SETFL, sflags & ~O_NONBLOCK);

	printf("\n\n[x] Received %lu bytes in total\n", total);
	arm9payload_size = total;

	close(clientfd);
	close(sockfd);

    // exit socket
    soc_exit();
    
    // transfer to file
    if (arm9payload_size) {
        printf("[x] Writing %s...\n", PAYLOAD_PATH);
        FILE* fp = fopen(PAYLOAD_PATH, "wb");
        if (fp == NULL) {
            printf("[!] Error: cannot open file\n");
            free(arm9payload_buf);
            return 0;
        }
        fwrite(arm9payload_buf, arm9payload_size, 1, fp);
        fclose(fp);
        printf("[x] Success!\n");
    }
    free(arm9payload_buf);
    
	return (arm9payload_size != 0);
}

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
    acInit();
	hidInit();
    gfxInitDefault();
	gfxSwapBuffers(); 
    gfxSet3D(false);
    consoleInit(GFX_TOP, NULL);
    
    printf("[+] A9LH Netload Companion v0.0.1\n\n");
    while (true) {
        if (recv_arm9_payload()) {
            printf("\n[x] Now rebooting...\n");
            quick_reboot();
            break;
        } else {
            wait_any_key();
            break;
        }
    }
    
    // Deinitialize services
    gfxExit();
    hidExit();
    acExit();
    aptExit();
    srvExit();
    return 0;
}
