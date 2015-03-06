#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <3ds.h>

#include "ftp.h"
#include "ftp_cmd.h"

#define DATA_BUFFER_SIZE (512*1024)

static char tmpStr[4096];
static u32 dataBuffer[DATA_BUFFER_SIZE/4];
// Used to track the last RNFR command sent
static char renameSource[4096];

static void unicodeToChar(char* dst, u16* src)
{
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void ftp_cmd_PWD(int s, char* cmd, char* arg)
{
	siprintf(tmpStr, "\"%s\"",currentPath);
	ftp_sendResponse(s, 257, tmpStr);
}

void ftp_cmd_PASV(int s, char* cmd, char* arg)
{
	sprintf(tmpStr, "Entering Passive Mode (%d,%d,%d,%d,%d,%d).", (int)(currentIP&0xFF), (int)((currentIP>>8)&0xFF), (int)((currentIP>>16)&0xFF), (int)(currentIP>>24), dataPort>>8, dataPort&0xFF);
	ftp_sendResponse(s, 227, tmpStr);
}

void ftp_cmd_LIST(int s, char* cmd, char* arg)
{
	int data_s=ftp_openDataChannel();
	ftp_sendResponse(s, 150, "opening ASCII data channel");

	//send LIST data
	Handle dirHandle;
	FS_path dirPath=FS_makePath(PATH_CHAR, currentPath);
	FSUSER_OpenDirectory(NULL, &dirHandle, sdmcArchive, dirPath);

	u32 entriesRead=0;
	do{
		u16 entryBuffer[512];
		char data[256];
		FSDIR_Read(dirHandle, &entriesRead, 1, (FS_dirent*)entryBuffer);
		if(!entriesRead)break;
		unicodeToChar(data, entryBuffer);
		siprintf((char*)entryBuffer, "%crwxrwxrwx   2 3DS        %d Feb  1  2009 %s\r",entryBuffer[0x21c/2]?'d':'-',entryBuffer[0x220/2]|(entryBuffer[0x222/2]<<16),data);
		
		send(data_s, entryBuffer, strlen((char*)entryBuffer), 0);
	}while(entriesRead>0);
	u8 endByte=0x0;
	send(data_s, &endByte, 1, 0);
	FSDIR_Close(dirHandle);

	closesocket(data_s);
	ftp_sendResponse(s, 226, "transfer complete");
}

void ftp_cmd_MKD(int s, char* cmd, char* arg)
{
	sprintf(tmpStr, "%s%s", currentPath, arg);

	int ret;
	ret=FSUSER_CreateDirectory(NULL, sdmcArchive, FS_makePath(PATH_CHAR, tmpStr));
	if(ret == PATH_EXISTS){
		sprintf(shared_ftp," directory exists %s", tmpStr);
		ftp_sendResponse(s, 521, "directory exists; no action");
	}else{
		sprintf(shared_ftp," create directory result %s (%08X)", tmpStr, ret);
		ftp_sendResponse(s, 257, "created directory");
	}
}

void ftp_cmd_RMD(int s, char* cmd, char* arg)
{
	sprintf(tmpStr, "%s%s", currentPath, arg);
	
	int ret;
	ret=FSUSER_DeleteDirectory(NULL, sdmcArchive, FS_makePath(PATH_CHAR, arg));
	sprintf(shared_ftp," delete result %s (%08X)", arg, ret);
	ftp_sendResponse(s, 250, "delete completed");
}

void ftp_cmd_DELE(int s, char* cmd, char* arg)
{
	sprintf(tmpStr, "%s%s", currentPath, arg);
	
	int ret;
	ret=FSUSER_DeleteFile(NULL, sdmcArchive, FS_makePath(PATH_CHAR, arg));
	sprintf(shared_ftp," delete result %s (%08X)", arg, ret);
	ftp_sendResponse(s, 250, "delete completed");
}

void ftp_cmd_STOR(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 150, "opening binary data channel");
	int data_s=ftp_openDataChannel();
	int ret;

	//Create the currentPath if it does not exist.
	ret=FSUSER_CreateDirectory(NULL, sdmcArchive, FS_makePath(PATH_CHAR, currentPath));
	if(ret == PATH_EXISTS){
		sprintf(shared_ftp," directory exists %s", currentPath);
	}else{
		sprintf(shared_ftp," create directory result %s (%08X)", currentPath, ret);
	}

	sprintf(tmpStr, "%s%s",currentPath,arg);
	Handle fileHandle;
	ret=FSUSER_OpenFile(NULL, &fileHandle, sdmcArchive, FS_makePath(PATH_CHAR, tmpStr), FS_OPEN_WRITE|FS_OPEN_CREATE, 0);
	sprintf(shared_ftp,"  storing %s (%08X)", tmpStr, ret);
	u32 totalSize=0;
	while((ret=recv(data_s, dataBuffer, DATA_BUFFER_SIZE, 0))>0){FSFILE_Write(fileHandle, (u32*)&ret, totalSize, (u32*)dataBuffer, ret, 0x10001);totalSize+=ret;}
	FSFILE_Close(fileHandle);

	closesocket(data_s);
	ftp_sendResponse(s, 226, "transfer complete");
}

void ftp_cmd_RETR(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 150, "opening binary data channel");
	int data_s=ftp_openDataChannel();

	sprintf(tmpStr, "%s%s",currentPath,arg);
	sprintf(shared_ftp,"%s",tmpStr);
	Handle fileHandle;
	FSUSER_OpenFile(NULL, &fileHandle, sdmcArchive, FS_makePath(PATH_CHAR, tmpStr), FS_OPEN_READ, 0);
	int ret;
	u32 readSize=0;
	u32 totalSize=0;
	do{
		ret=FSFILE_Read(fileHandle, (u32*)&readSize, totalSize, (u32*)dataBuffer, DATA_BUFFER_SIZE);
		if(ret || !readSize)break;
		ret=send(data_s, dataBuffer, readSize, 0);
		totalSize+=readSize;
	}while(readSize && ret>0);
	FSFILE_Close(fileHandle);

	closesocket(data_s);
	ftp_sendResponse(s, 226, "transfer complete");
}

void ftp_cmd_USER(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 200, "password ?");
}

void ftp_cmd_PASS(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 200, "ok");
}

void ftp_cmd_CWD(int s, char* cmd, char* arg)
{
	//TODO : proper directory navigation code, with error reporting...
	if(arg[0]=='/')strcpy(currentPath,arg);
	else strcat(currentPath,arg);
	int l=strlen(currentPath);
	if(!l || currentPath[l-1]!='/')strcat(currentPath,"/");
	sprintf(shared_ftp,"  => %s", currentPath);
	ftp_sendResponse(s, 200, "ok");
}

void ftp_cmd_TYPE(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 200, "changed type");
}

void ftp_cmd_QUIT(int s, char* cmd, char* arg)
{
	ftp_sendResponse(s, 221, "disconnecting");
}

void ftp_cmd_RNFR(int s, char* cmd, char* arg)
{
	snprintf(renameSource, sizeof(renameSource), "%s", arg);
	ftp_sendResponse(s, 350, "send destination");
}

void ftp_cmd_RNTO(int s, char* cmd, char* arg)
{
	snprintf(tmpStr, sizeof(tmpStr), "%s", arg);

	int ret = FSUSER_RenameFile(NULL,
			sdmcArchive, FS_makePath(PATH_CHAR, renameSource),
			sdmcArchive, FS_makePath(PATH_CHAR, arg));
	sprintf(shared_ftp," rename result %s -> %s (%08X)", renameSource, arg, ret);

	if (ret != 0)
	{
		// If the file rename failed, try a directory rename in case that's
		// what the user is trying to do.
		ret = FSUSER_RenameDirectory(NULL,
				sdmcArchive, FS_makePath(PATH_CHAR, renameSource),
				sdmcArchive, FS_makePath(PATH_CHAR, arg));
		sprintf(shared_ftp," rename dir result %s -> %s (%08X)", renameSource, arg, ret);
	}

	if (ret == 0)
		ftp_sendResponse(s, 250, "rename completed");
	else
		ftp_sendResponse(s, 550, "rename failed");

	renameSource[0] = '\0';
}

ftp_cmd_s ftp_cmd[]=
{
	{"PWD", ftp_cmd_PWD},
	{"PASV", ftp_cmd_PASV},
	{"LIST", ftp_cmd_LIST},
	{"STOR", ftp_cmd_STOR},
	{"RETR", ftp_cmd_RETR},
	{"USER", ftp_cmd_USER},
	{"PASS", ftp_cmd_PASS},
	{"CWD", ftp_cmd_CWD},
	{"TYPE", ftp_cmd_TYPE},
	{"QUIT", ftp_cmd_QUIT},
	{"DELE", ftp_cmd_DELE},
	{"MKD",  ftp_cmd_MKD},
	{"RMD",  ftp_cmd_RMD},
	{"RNFR", ftp_cmd_RNFR},
	{"RNTO", ftp_cmd_RNTO},
};

int ftp_cmd_num = sizeof(ftp_cmd)/sizeof(*ftp_cmd);
