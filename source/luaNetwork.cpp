/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> ---------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> -------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib and ftpony src ----------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <3ds.h>
#include <fcntl.h>
#include "include/luaplayer.h"
#include "include/ftp/ftp.h"

extern bool ftp_state;
static int connfd;
typedef struct
{
	u32 magic;
	u32 sock;
	struct sockaddr_in addrTo;
	bool serverSocket;
} Socket;

static int lua_checkFTPcommand(lua_State *L){
	int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	if(connfd<0)connfd=ftp_getConnection();
		else{
			int ret=ftp_frame(connfd);
			if(ret==1)
			{
				sprintf(shared_ftp,"Client has disconnected. Wait for next client...");
				connfd=-1;
			}
	}
	lua_pushstring(L, shared_ftp);
	return 1;
}

static int lua_wifistat(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u32 wifiStatus;
	if (ACU_GetWifiStatus(NULL, &wifiStatus) ==  0xE0A09D2E) lua_pushboolean(L,false);
	else lua_pushboolean(L,wifiStatus);
	return 1;
}

static int lua_macaddr(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8* mac_byte = (u8*)0x1FF81060; 
	char mac_address[18];
	sprintf(mac_address,"%02X:%02X:%02X:%02X:%02X:%02X",*mac_byte,*(mac_byte+1),*(mac_byte+2),*(mac_byte+3),*(mac_byte+4),*(mac_byte+5));
	mac_address[17] = 0;
	lua_pushstring(L,mac_address);
	return 1;
}

static int lua_ipaddr(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u32 ip=(u32)gethostid();
	char ip_address[64];
	sprintf(ip_address,"%lu.%lu.%lu.%lu", ip & 0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF);
	lua_pushstring(L,ip_address);
	return 1;
}

static int lua_download(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char* url = luaL_checkstring(L,1);
	const char* file = luaL_checkstring(L,2);
	httpcContext context;
	Result ret = httpcOpenContext(&context, (char*)url , 0);
	if(ret==0){
		httpcBeginRequest(&context);
		/*httpcReqStatus loading;
		httpcGetRequestState(&context, &loading);
		while (loading == 0x5){
			httpcGetRequestState(&context, &loading);
		}*/
		u32 statuscode=0;
		u32 contentsize=0;
		httpcGetResponseStatusCode(&context, &statuscode, 0);
		if (statuscode != 200) luaL_error(L, "download request error");
		httpcGetDownloadSizeState(&context, NULL, &contentsize);
		u8* buf = (u8*)malloc(contentsize);
		memset(buf, 0, contentsize);
		httpcDownloadData(&context, buf, contentsize, NULL);
		Handle fileHandle;
		u32 bytesWritten;
		FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
		FS_path filePath=FS_makePath(PATH_CHAR, file);
		FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
		FSFILE_Write(fileHandle, &bytesWritten, 0, buf, contentsize,0x10001);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		free(buf);
	}else luaL_error(L, "error opening url");
	httpcCloseContext(&context);
	return 0;
}

static int lua_downstring(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char* url = luaL_checkstring(L,1);
	httpcContext context;
	Result ret = httpcOpenContext(&context, (char*)url , 0);
	if(ret==0){
		httpcBeginRequest(&context);
		/*httpcReqStatus loading;
		httpcGetRequestState(&context, &loading);
		while (loading == 0x5){
			httpcGetRequestState(&context, &loading);
		}*/
		u32 statuscode=0;
		u32 contentsize=0;
		httpcGetResponseStatusCode(&context, &statuscode, 0);
		char text[128];
		sprintf(text,"%i",statuscode);
		if (statuscode != 200) luaL_error(L, text);
		httpcGetDownloadSizeState(&context, NULL, &contentsize);
		unsigned char *buffer = (unsigned char*)malloc(contentsize+1);
		httpcDownloadData(&context, buffer, contentsize, NULL);
		buffer[contentsize] = 0;
		lua_pushlstring(L,(const char*)buffer,contentsize);
		free(buffer);
	}else luaL_error(L, "error opening url");
	httpcCloseContext(&context);
	return 1;
}

static int SpaceCounter(char* string){
	int res=0;
	while (*string){
		if (string[0] == 0x20) res++;
		string++;
	}
	return res;
}

static int lua_sendmail(lua_State *L){ //BETA func
	int argc = lua_gettop(L);
	if (argc != 3) return luaL_error(L, "wrong number of arguments");
	char* to = (char*)luaL_checkstring(L,1);
	char* subj = (char*)luaL_checkstring(L,2);
	char* mex = (char*)luaL_checkstring(L,3);
	int req_size = 70;
	int nss = SpaceCounter(subj);
	int nsm = SpaceCounter(mex);
	req_size = req_size + nss * 2 + nsm * 2 + strlen(subj) + strlen(mex) + strlen(to);
	char* url = (char*)malloc(req_size);
	strcpy(url,"http://rinnegatamante.netsons.org/tmp_mail_lpp_beta.php?t=");
	strcat(url,to);
	strcat(url,"&s=");
	char* subj_p = subj;
	char* url_p = &url[strlen(url)];
	while (*subj_p){
		if (subj_p[0] == 0x20){
			url_p[0] = '%';
			url_p++;
			url_p[0] = '2';
			url_p++;
			url_p[0] = '0';
		}else url_p[0] = subj_p[0];
		url_p++;
		subj_p++;
	}
	strcat(url,"&b=");
	char* mex_p = mex;
	url_p = &url[strlen(url)];
	while (*mex_p){
		if (mex_p[0] == 0x20){
			url_p[0] = '%';
			url_p++;
			url_p[0] = '2';
			url_p++;
			url_p[0] = '0';
		}else url_p[0] = mex_p[0];
		url_p++;
		mex_p++;
	}
	url_p[0] = 0;
	httpcContext context;
	Result ret = httpcOpenContext(&context, (char*)url , 0);
	if(ret==0){
		httpcBeginRequest(&context);
		httpcReqStatus loading;
		httpcGetRequestState(&context, &loading);
		while (loading == 0x5){
			httpcGetRequestState(&context, &loading);
		}
		u32 statuscode=0;
		u32 contentsize=0;
		httpcGetResponseStatusCode(&context, &statuscode, 0);
		if (statuscode != 200) luaL_error(L, "request error");
		httpcGetDownloadSizeState(&context, NULL, &contentsize);
		u8 response;
		httpcDownloadData(&context, &response, contentsize, NULL);
		lua_pushboolean(L,response);
		free(url);
	}else luaL_error(L, "error opening url");
	httpcCloseContext(&context);
	return 1;
}

static int lua_wifilevel(lua_State *L){
	int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8* wifi_link = (u8*)0x1FF81066;
	lua_pushinteger(L,*wifi_link);
	return 1;
}

static int lua_initSock(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	ftp_init();
	ftp_state = true;
	sprintf(shared_ftp,"Waiting for connection...");
	connfd = -1;
	return 0;
}

static int lua_createServerSocket(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1) 
	return luaL_error(L, "Socket.createServerSocket(port) takes one argument.");
	int port = luaL_checkinteger(L, 1);

	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	my_socket->serverSocket = true;

	my_socket->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (my_socket->sock <= 0) {
		return luaL_error(L, "invalid socket.");
	}

	my_socket->addrTo.sin_family = AF_INET;
	my_socket->addrTo.sin_port = htons(port);
	my_socket->addrTo.sin_addr.s_addr = 0;

	int err = bind(my_socket->sock, (struct sockaddr*)&my_socket->addrTo, sizeof(my_socket->addrTo));
	if (err != 0) {
		return luaL_error(L, "bind error.");
	}

	fcntl(my_socket->sock, F_SETFL, O_NONBLOCK);

	err = listen(my_socket->sock, 1);
	if (err != 0) {
		return luaL_error(L, "listen error.");
	}
	
	my_socket->magic = 0xDEADDEAD;
	lua_pushinteger(L,(u32)my_socket);
	return 1;
}

static int lua_shutSock(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	ftp_exit();
	ftp_state = false;
	return 0;
}

static int lua_recv(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");

	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	u32 size = luaL_checkinteger(L, 2);
	
	#ifndef SKIP_ERROR_HANDLING
		if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (my_socket->serverSocket) return luaL_error(L, "recv not allowed for server sockets.");

	char* data = (char*)malloc(size);
	int count = recv(my_socket->sock, data, size, 0);
	if (count > 0) lua_pushlstring(L, data, count);
	else lua_pushstring(L, "");
	return 1;
}

static int lua_send(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");

	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	size_t size;
	char* text = (char*)luaL_checklstring(L, 2, &size);

	#ifndef SKIP_ERROR_HANDLING
		if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (my_socket->serverSocket) return luaL_error(L, "send not allowed for server sockets.");
	if (!text) return luaL_error(L, "Socket.send() expected a string.");
	
	int result = sendto(my_socket->sock, text, size, 0, NULL, 0);
	lua_pushinteger(L, result);
	return 1;
}

static int lua_connect(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");

	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	my_socket->serverSocket = false;
	my_socket->magic = 0xDEADDEAD;
	
	const char *host = luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	my_socket->addrTo.sin_family = AF_INET;
	my_socket->addrTo.sin_port = htons(port);
	my_socket->addrTo.sin_addr.s_addr = inet_addr(host);

	my_socket->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (my_socket->sock < 0) return luaL_error(L, "Failed creating socket.");
	fcntl(my_socket->sock, F_SETFL, O_NONBLOCK);

	int err = connect(my_socket->sock, (struct sockaddr*)&my_socket->addrTo, sizeof(my_socket->addrTo));
	if (err < 0 ) return luaL_error(L, "Failed connecting server.");
	
	lua_pushinteger(L, (u32)my_socket);
	return 1;
}

static int lua_accept(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");

	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	
	#ifndef SKIP_ERROR_HANDLING
		if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (!my_socket->serverSocket) return luaL_error(L, "accept allowed for server sockets only.");

	struct sockaddr_in addrAccept;
	socklen_t cbAddrAccept = sizeof(addrAccept);
	u32 sockClient = accept(my_socket->sock, (struct sockaddr*)&addrAccept, &cbAddrAccept);
	if (sockClient <= 0) {
		return 0;
	}

	Socket* incomingSocket = (Socket*) malloc(sizeof(Socket));
	incomingSocket->serverSocket = 0;
	incomingSocket->sock = sockClient;
	incomingSocket->addrTo = addrAccept;
	incomingSocket->magic = 0xDEADDEAD;
	lua_pushinteger(L, (u32)incomingSocket);
	return 1;
}

static int lua_closeSock(lua_State *L)
{
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "Socket.close() takes one argument.");

	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	
	#ifndef SKIP_ERROR_HANDLING
		if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	closesocket(my_socket->sock);
	free(my_socket);
	return 0;
}

//Register our Network Functions
static const luaL_Reg Network_functions[] = {
  {"updateFTP",				lua_checkFTPcommand},
  {"isWifiEnabled",			lua_wifistat},
  {"getWifiLevel",			lua_wifilevel},
  {"getMacAddress",			lua_macaddr},
  {"getIPAddress",			lua_ipaddr},
  {"downloadFile",			lua_download},
  {"requestString",			lua_downstring},
  {"sendMail",				lua_sendmail},
  {0, 0}
};

//Register our Socket Functions
static const luaL_Reg Socket_functions[] = {
  {"init",				lua_initSock},
  {"term",				lua_shutSock},
  {"createServerSocket",lua_createServerSocket},
  {"connect",			lua_connect},
  {"receive",			lua_recv},
  {"send",				lua_send},
  {"accept",			lua_accept},
  {"close",				lua_closeSock},
  {0, 0}
};

void luaNetwork_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Network_functions, 0);
	lua_setglobal(L, "Network");
	lua_newtable(L);
	luaL_setfuncs(L, Socket_functions, 0);
	lua_setglobal(L, "Socket");
}