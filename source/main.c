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

#define APP_NAME "ARM9 Netload Companion v0.1.3"

#define PAYLOAD_PATH_GM9 "/gm9/payloads"
#define PAYLOAD_PATH_LUMA "/luma/payloads"

#define NETWORK_PORT 17491
#define ARM9_PAYLOAD_MAX_SIZE 0x80000
#define ZLIB_CHUNK (16 * 1024)

void write_to_file(const char* filename, u8* buf, u32 size) {
    printf("[x] Writing %s...\n", filename);
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
        printf("[!] Error: cannot open file\n");
    } else {
        fwrite(buf, size, 1, fp);
        fclose(fp);
    }
}

s32 recv_data (int sockfd, void *buf, size_t len, bool recv_all) {
    u32 total = 0;
    do {
        s32 failcnt = 0;
        time_t lastrcvtime = time(NULL);
        s32 recvd = 0;
        while (aptMainLoop() && ((recvd = recv(sockfd, buf + total, len - total, 0)) < 0)) {
            failcnt++;
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("\n[!] Error: netloader error\n");
                return -1;
            }
            if ((failcnt >= 0x10000) && (time(NULL) - lastrcvtime >= 10)) {
                printf("\n[!] Error: netloader timeout\n");
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
    u32 wifi = 0;
    u32 gm9launch = 1;
    
    // init socket
    soc_init();

    // wait for wifi to be available
    // from: https://github.com/mtheall/ftpd/blob/master/source/ftp.c#L1414
    if ((ACU_GetWifiStatus(&wifi) != 0) || (!wifi)) {
        wifi = 0;
        printf("[x] Waiting for Wifi...\n");
        while(aptMainLoop() && !wifi) {
            hidScanInput();
            if(hidKeysDown() & KEY_B) {
                printf("[!] Aborted\n");
                return 0;
            }

            /* update the wifi status */
            if (ACU_GetWifiStatus(&wifi) != 0) wifi = 0;
        }
        if (!wifi)
            return 0;
    }

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
    printf("[?] mode: %s (set with R)\r", (gm9launch) ? "gm9" : "ask");

	sflags = fcntl(sockfd, F_GETFL);
	if (sflags == -1) {
		printf("[!] Error: fcntl() (1)\n");
		close(sockfd);
	}
	fcntl(sockfd, F_SETFL, sflags | O_NONBLOCK);

	hidScanInput();
    do {
		hidScanInput();
		if (hidKeysDown() & KEY_B) {
            printf("                               \r");
			printf("[!] Aborted\n");
			close(sockfd);
			return 0;
		} else if (hidKeysDown() & KEY_R) {
            gm9launch = !gm9launch;
            printf("[?] mode: %s (set with R)   \r", (gm9launch) ? "gm9" : "ask");
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
    u8 *buf = (u8*) linearMemAlign(512 + ARM9_PAYLOAD_MAX_SIZE, 0x400000);
    // u8* buf = (u8*) malloc(512 + ARM9_PAYLOAD_MAX_SIZE);
    if (!buf) {
        printf("[!] Error: out of memory\n");
        close(sockfd);
        close(clientfd);
        return 0;
    }
    u8* filename = buf;
    u8* destname = buf + 256;
    u8* arm9payload_buf = buf + 512;
    
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
    
    // flush data cache
    GSPGPU_FlushDataCache(buf, 512 + ARM9_PAYLOAD_MAX_SIZE);

    // transfer to file
    if (arm9payload_size) {
        if (!gm9launch) {
            printf("\n[+] A to write to FCRAM\n");
            printf("[+] L to write /bootonce.firm\n");
            printf("[+] R to write %s/%s\n", PAYLOAD_PATH_GM9, filename);
            printf("[+] \x1b to write %s/left_A9NC.firm\n", PAYLOAD_PATH_LUMA);
            printf("[+] ? to write %s/?_%s\n", PAYLOAD_PATH_LUMA, filename);
            printf("[+] B to quit\n");
        }
        do {
            u32 pad_state = (gm9launch) ? KEY_A : wait_key();
            if (pad_state & KEY_B) {
                printf("[x] Cancelled\n");
                arm9payload_size = -1;
                break;
            } else if (pad_state & KEY_A) {
                *destname = '\0';
                snprintf((char*) buf, 255, "A9NC"); // magic number
            } else if (pad_state & KEY_L) {
                snprintf((char*) destname, 255, "/bootonce.firm");
            } else if (pad_state & KEY_R) {
                snprintf((char*) destname, 255, "%s/%s", PAYLOAD_PATH_GM9, (char*) filename);
            } else if (pad_state & KEY_LEFT) {
                snprintf((char*) destname, 255, "%s/left_A9NC.firm", PAYLOAD_PATH_LUMA);
            } else if (pad_state & KEY_START) {
                snprintf((char*) destname, 255, "%s/start_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else if (pad_state & KEY_RIGHT) {
                snprintf((char*) destname, 255, "%s/right_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else if (pad_state & KEY_UP) {
                snprintf((char*) destname, 255, "%s/up_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else if (pad_state & KEY_DOWN) {
                snprintf((char*) destname, 255, "%s/down_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else if (pad_state & KEY_X) {
                snprintf((char*) destname, 255, "%s/x_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else if (pad_state & KEY_Y) {
                snprintf((char*) destname, 255, "%s/y_%s", PAYLOAD_PATH_LUMA, (char*) filename);
            } else {
                continue;
            }
            if (*destname) write_to_file((char*) destname, arm9payload_buf, arm9payload_size);
            printf("[x] Success!\n");
        } while(false);
    }
    
	return arm9payload_size;
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
    
    printf("[+] %s\n\n", APP_NAME);
    s32 res = recv_arm9_payload();
    if (res > 0) APT_HardwareResetAsync(); // reboot
    else if (res == 0) wait_any_key();
    
    // Deinitialize services
    gfxExit();
    hidExit();
    acExit();
    aptExit();
    srvExit();
    return 0;
}
