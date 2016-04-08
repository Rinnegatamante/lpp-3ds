#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <3ds.h>
#include <fcntl.h>

#include "ftp.h"
#include "ftp_cmd.h"
#define sprint sprintf

FS_Archive sdmcArchive;

char tmpBuffer[512];
const int commandPort=5000;
int dataPort=5001;
char currentPath[4096];
u32 currentIP;
u32* ftp_mem;
static int sock_buffersize = 32768;

int listenfd;

char shared_ftp[256];

void ftp_init()
{
	Result ret;
	//ret=fsInit();
	//sprint(shared_ftp,"fsInit %08X", (unsigned int)ret);

	sdmcArchive=(FS_Archive){0x00000009, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(&sdmcArchive);
	sprint(shared_ftp,"FSUSER_OpenArchive %08X", (unsigned int)ret);
	
	ftp_mem = (u32*)memalign(0x1000, 0x100000);
	ret=socInit(ftp_mem, 0x100000);
	sprint(shared_ftp,"SOC_Initialize %08X", (unsigned int)ret);

	sprintf(currentPath, "/");

	currentIP=(u32)gethostid();

	listenfd=-1;
}

int cmd_sock = -1;

void ftp_exit()
{
	if (listenfd >= 0){
		closesocket(listenfd);
		listenfd = -1;
	}
	if (cmd_sock >= 0){
		ftp_sendResponse(cmd_sock, 200, "bye bye");
		closesocket(cmd_sock);
		cmd_sock = -1;
	}
	socExit();
	free(ftp_mem);
}

int ftp_openCommandChannel()
{
	if(listenfd<0)
	{
		struct sockaddr_in serv_addr;

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		memset(&serv_addr, '0', sizeof(serv_addr));

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = gethostid();
		serv_addr.sin_port = htons(commandPort); 

		bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 
		int yes = 1;
		setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &sock_buffersize, sizeof(sock_buffersize));
		int flags = fcntl(listenfd, F_GETFL, 0);
		fcntl(listenfd, F_SETFL, O_NONBLOCK);

		listen(listenfd, 10); 
	}
	
	if (cmd_sock < 0) cmd_sock=accept(listenfd, (struct sockaddr*)NULL, NULL);
	if(cmd_sock>=0)
	{
		//closesocket(listenfd);
		//listenfd=-1;
		setsockopt(cmd_sock, SOL_SOCKET, SO_RCVBUF, &sock_buffersize, sizeof(sock_buffersize));
		int flags = fcntl(cmd_sock, F_GETFL, 0);	
		fcntl(cmd_sock, F_SETFL, flags | O_NONBLOCK);
	}

	return cmd_sock;
}

int ftp_openDataChannel()
{
	int listenfd;
	struct sockaddr_in serv_addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(dataPort); 

	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	listen(listenfd, 10); 
	
	int ret=accept(listenfd, (struct sockaddr*)NULL, NULL);
	setsockopt(ret, SOL_SOCKET, SO_RCVBUF, &sock_buffersize, sizeof(sock_buffersize));
	closesocket(listenfd);

	return ret;
}

int ftp_sendResponse(int s, int n, char* mes)
{
	char data[128];
	sprintf(data, "%d %s\r", n, mes);
	return send(s,data,strlen(data),0);
}

int linelen(char* str)
{
	int i=0; while(*str && *str!='\n' && *str!='\r'){i++;str++;}
	*str=0x0;
	return i;
}

int ftp_processCommand(int s, char* data)
{
	if(!data)return -1;
	int n=linelen(data);
	char cmd[5];
	char arg[256]="";
	if(n>2 && (!data[3] || data[3]==' ' || data[3]=='\n' || data[3]=='\r')){memcpy(cmd,data,3);cmd[3]=0x0; if(n>3)memcpy(arg, &data[4], n-4);}
	else if(n>3 && (!data[4] || data[4]==' ' || data[4]=='\r' || data[4]=='\n')){memcpy(cmd,data,4);cmd[4]=0x0; if(n>4)memcpy(arg, &data[5], n-5);}
	else return -1;

	sprint(shared_ftp,"received command : %s (%s)",cmd,arg);

	int i;
	for(i=0; i<ftp_cmd_num; i++)if(!strcmp(cmd, ftp_cmd[i].name)){ftp_cmd[i].handler(s, cmd, arg); break;}
	if(i>=ftp_cmd_num)ftp_sendResponse(s, 502, "invalid command");
	return 0;
}

int ftp_frame(int s)
{
	int cmd_sock_2=accept(listenfd, (struct sockaddr*)NULL, NULL);
	if (cmd_sock_2 >= 0){ // TODO: A proper implementation with poll could be much more better
		sprint(shared_ftp,"received connection ! %dgreeting...",cmd_sock_2);
		ftp_sendResponse(cmd_sock_2, 200, "hello");
		char buffer[512];
		memset(buffer, 0, 512);
		int ret=recv(cmd_sock_2,buffer,512,0);
		ftp_processCommand(cmd_sock_2,buffer);
		closesocket(cmd_sock);
		cmd_sock = cmd_sock_2;
		s = cmd_sock;
	}
	char buffer[512];
	memset(buffer, 0, 512);
	int ret=recv(s,buffer,512,0);
	if(!ret){
		closesocket(cmd_sock);
		cmd_sock = -1;
		return 1; //client has disconnected
	}else return ftp_processCommand(s,buffer);
}

int ftp_getConnection()
{
	int connfd = ftp_openCommandChannel();
	if(connfd>=0)
	{
		sprint(shared_ftp,"received connection ! %dgreeting...",connfd);
		ftp_sendResponse(connfd, 200, "hello");
	}
	return connfd;
}
