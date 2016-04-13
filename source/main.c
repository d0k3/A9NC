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
#include <time.h>
#include <zlib.h>
#include "sochlp.h"
#include "hid.h"

#define PAYLOAD_PATH "/arm9testpayload.bin"
#define NETWORK_PORT 17491
#define ARM9_PAYLOAD_MAX_SIZE 0x80000
#define ZLIB_CHUNK (16 * 1024)

s32 recv_data (int sockfd, void *buf, size_t len, bool recv_all) {
    u32 total = 0;
    do {
        s32 failcnt = 0;
        time_t lastrcvtime = time(NULL);
        s32 recvd = 0;
        while (aptMainLoop() && ((recvd = recv(sockfd, buf + total, len - total, 0)) < 0)) {
            failcnt++;
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("[!] Error: netloader error\n");
                return -1;
            }
            if ((failcnt >= 0x8000) && (time(NULL) - lastrcvtime >= 3)) {
                printf("[!] Error: netloader timeout\n");
                return -1;
            }
        }
        total += recvd;
    } while (aptMainLoop() && recv_all && (total < len));
    return total;
}

// adapted from https://github.com/smealum/3ds_hb_menu/blob/master/source/netloader.c#L86
int recv_zlib_chunks (int sockfd, void *buf, size_t len) {
    z_stream strm  = {0};
    
    strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;

    size_t total_rcvd = 0;
    int err = -1;

    err = inflateInit(&strm);
	if (err != Z_OK) {
		printf("[!] Error: inflateInit()\n");
		return err;
	}
    
    do {
        u8 in[ZLIB_CHUNK];
        size_t chunksize;
        
        if (recv_data(sockfd, &chunksize, 4, 1) != 4) break;
        if (chunksize < 0 || chunksize > ZLIB_CHUNK) {
            inflateEnd(&strm);
            printf("[!] Error: bad chunk size\n");
            return Z_DATA_ERROR;
        }
        if ((strm.avail_in = recv_data(sockfd, in, chunksize, 1)) != chunksize) break;
        strm.next_in = in;
        do {
            strm.avail_out = ZLIB_CHUNK;
            strm.next_out = buf + total_rcvd;
            err = inflate(&strm, Z_NO_FLUSH);
            if (err < 0) {
                inflateEnd(&strm);
                printf("[!] Error: inflate()\n");
                return err;
            }
            total_rcvd += ZLIB_CHUNK - strm.avail_out;
            printf("[x] Received: %u byte / %u byte\r", total_rcvd, len);
        } while (aptMainLoop() && strm.avail_out == 0);
    } while (aptMainLoop() && err != Z_STREAM_END);
    printf("[x] Received: %u byte / %u byte\n", total_rcvd, len);
    inflateEnd(&strm);
    return (err == Z_STREAM_END) ? total_rcvd : Z_DATA_ERROR;
}

// adapted from: https://github.com/patois/Brahma/blob/master/source/brahma.c#L168-L259
s32 recv_arm9_payload (void) {
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
	} while (aptMainLoop());

	printf("[x] Connection from %s:%d\n", inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port));

    
    s32 filename_size = 0;
	s32 arm9payload_size = 0;
    s32 command_size = 0;
    u8* buf = (u8*) malloc(256 + ARM9_PAYLOAD_MAX_SIZE);
    if (!buf) {
        printf("[!] Error: out of memory\n");
        close(sockfd);
        close(clientfd);
        return 0;
    }
    u8* filename = buf;
    u8* arm9payload_buf = buf + 256;
    
    do { // handle 3DSlink transfer header and data
        if (recv_data(clientfd, &filename_size, 4, 1) != 4) break;
        if (filename_size < 0 || filename_size >= 256) {
            printf("[!] Error: bad header\n");
        }
        if (recv_data(clientfd, filename, filename_size, 1) != filename_size) break;
        printf("[x] Receiving \"%s\"\n", filename);
        if (recv_data(clientfd, &arm9payload_size, 4, 1) != 4) break;
        if (arm9payload_size < 0 || arm9payload_size >= ARM9_PAYLOAD_MAX_SIZE) {
			printf("[!] Error: invalid payload size\n");
            arm9payload_size = 0;
			break;
		}
        int response = 0;
        send(clientfd, (int*) &response, 4, 0);
        if (recv_zlib_chunks(clientfd, arm9payload_buf, arm9payload_size) !=
            arm9payload_size) {
            arm9payload_size = 0;
            printf("[!] Error: corrupt transfer\n");
            break;
        }
        send(clientfd, (int*) &response, 4, 0);
        if (recv_data(clientfd, &command_size, 4, 1) != 4) break;
        // any command line argument is ignored at that point
    } while(false);

	fcntl(sockfd, F_SETFL, sflags & ~O_NONBLOCK);

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
            free(buf);
            return 0;
        }
        fwrite(arm9payload_buf, arm9payload_size, 1, fp);
        fclose(fp);
        printf("[x] Success!\n");
    }
    free(buf);
    
	return (arm9payload_size != 0);
}

// adapted from: https://github.com/AlbertoSONIC/3DS_Quick_Reboot/blob/master/source/main.c
void quick_reboot (void) {
    //Reboot Code
    aptOpenSession();
    APT_HardwareResetAsync();
    aptCloseSession();
}

int main () {
    // Initialize services
	srvInit();
	aptInit();
    acInit();
	hidInit();
    gfxInitDefault();
	gfxSwapBuffers(); 
    gfxSet3D(false);
    consoleInit(GFX_TOP, NULL);
    
    printf("[+] A9LH Netload Companion v0.0.3\n\n");
    if (recv_arm9_payload()) {
        printf("\n[x] Now rebooting...\n");
        quick_reboot();
    } else wait_any_key();
    
    // Deinitialize services
    gfxExit();
    hidExit();
    acExit();
    aptExit();
    srvExit();
    return 0;
}
