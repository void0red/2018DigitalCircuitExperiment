#include "hps_0.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"

#define MESSMAXSIZE 1024
#define LWH2FSPAN 0x200000
#define H2FSPAN   0X100000


static const uint32_t init_data[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
static const uint32_t k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};


int handle(char* buf);
void write_word(uint32_t *des, uint32_t *src, int size);
void sig_handler(int sig);

int stop = 0;

int main()
{

	signal(SIGIO, sig_handler);

	//read and handle data
	char *buf = (char*)malloc(MESSMAXSIZE*sizeof(char));
	if(!buf){
		printf("Error: malloc error\n");
		return -1;
	}

	printf("###Read: ");
	fgets(buf, MESSMAXSIZE, stdin);
	int length = handle(buf);


	//init bridge and set mem
	int fd = open("/dev/mem", (O_RDWR | O_SYNC));
	if(fd == -1)
	{
		printf("Error: could not open \"/dev/mem\"...\n");
		return -1;
	}
	void *lwh2f = mmap(NULL, LWH2FSPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, ALT_LWFPGASLVS_OFST);
	void *h2f = mmap(NULL, H2FSPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd,  ALT_H2F_OFST);
	if(lwh2f == MAP_FAILED || h2f == MAP_FAILED)
	{
		printf("Error: mmap() failed...\n");
		close(fd);
		return -1;
	}


	//write message
	write_word((h2f+M_BASE), (uint32_t*)buf, length/4);

	//write K
	write_word((h2f+K_BASE), (uint32_t*)k, 64);

	//write group_H
	alt_write_word(lwh2f+A_INIT_BASE, init_data);
	alt_write_word(lwh2f+B_INIT_BASE, init_data+1);
	alt_write_word(lwh2f+C_INIT_BASE, init_data+2);
	alt_write_word(lwh2f+D_INIT_BASE, init_data+3);
	alt_write_word(lwh2f+E_INIT_BASE, init_data+4);
	alt_write_word(lwh2f+F_INIT_BASE, init_data+5);
	alt_write_word(lwh2f+G_INIT_BASE, init_data+6);
	alt_write_word(lwh2f+H_INIT_BASE, init_data+7);

	//write cnt
	alt_setbits_hword(lwh2f+CNT_N_BASE, (uint16_t)(length/64));

	//start
	alt_setbits_hword(lwh2f+START_BASE, 0xffff);


	//wait for interrupt
	int tpfd = open("/dev/stop", O_RDWR);
	if(tpfd == -1)
	{
		printf("Error: could not open \"/dev/stop\"...\n");
		close(fd);
		return -1;
	}


	//error: cant find the definition for the descriptor



	// fcntl(tpfd, F_SETOWN, getpid());
	// fcntl(tpfd, F_SETFL, fcntl(tpfd, F_GETFL) | FASYNC);


	while(!stop)
	{
		printf("wait for stop\n");
		usleep(1000*100);
	}

	if(stop)
		close(tpfd);


	//read results
	uint32_t results[8];
	results[0] = alt_read_word(lwh2f+A_BASE);
	results[1] = alt_read_word(lwh2f+B_BASE);
	results[2] = alt_read_word(lwh2f+C_BASE);
	results[3] = alt_read_word(lwh2f+D_BASE);
	results[4] = alt_read_word(lwh2f+E_BASE);
	results[5] = alt_read_word(lwh2f+F_BASE);
	results[6] = alt_read_word(lwh2f+G_BASE);
	results[7] = alt_read_word(lwh2f+H_BASE);

	for (int i = 0; i < 8; ++i)
	{
		printf("%x\n", results[i]);
	}
	printf("\n");

	int state1 = munmap(lwh2f, ALT_LWFPGASLVS_OFST);
	int state2 = munmap(h2f, ALT_H2F_OFST);
	if(state1 != 0 || state2 != 0)
	{
		printf("Error: munmap() failed...\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}
//(a+x)%64 == 56

int handle(char* buf)
{
	int sum = 0;
	while(*buf)
		sum += 1;
	realloc(*buf, sum + 64 + 8 + 1);
	memset(buf+sum, '\x00', 64 + 8);
	buf[sum] = 0x80;
	int tmp = 56-sum%64;
	int padding_size = tmp + (tmp < 0 ? 64 : 0);
	uint64_t length = sum * 8;
	for(int i = 0; i < 8; i++)
	{
		buf[sum+padding_size+i] = (length >> (7-i)*8) & 0x00000000000000ff;
		length >>= (7-i)*8;
	}
	return sum + padding_size + 8;
}


//32bits
void write_word(uint32_t *des, uint32_t *src, int size)
{
	for(int i = 0; i < size; i++)
	{
		alt_write_word(des+i, src+i);
	}
}

void sig_handler(int sig)
{
	if(sig == SIGIO)
	{
		stop = 1;
	}
}