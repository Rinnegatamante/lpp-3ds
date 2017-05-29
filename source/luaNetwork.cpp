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
#- xerpi for sf2dlib ---------------------------------------------------------------------------------------------------#
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
#include "include/utils.h"
#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

extern bool ftp_state;
static int connfd;
typedef struct{
	u32 magic;
	int sock;
	bool serverSocket;
	sslcContext sslc_context;
	bool isSSL;
} Socket;

u32 RootCertChain_contexthandle=0;

static int lua_checkFTPcommand(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 wifiStatus;
	if (ACU_GetWifiStatus( &wifiStatus) ==  0xE0A09D2E) lua_pushboolean(L,false);
	else lua_pushboolean(L,wifiStatus);
	return 1;
}

static int lua_macaddr(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc < 2 || argc > 5) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* url = luaL_checkstring(L,1);
	const char* file = luaL_checkstring(L,2);
	const char* headers = (argc >= 3) ? luaL_checkstring(L,3) : NULL;
	u8 method = (argc >= 4) ? luaL_checkinteger(L,4) : 0;
	const char* postdata = (argc >= 5) ? luaL_checkstring(L,5) : NULL;
	httpcContext context;
	u32 statuscode=0;
	HTTPC_RequestMethod useMethod = HTTPC_METHOD_GET;

	if(method <= 3 && method >= 1) useMethod = (HTTPC_RequestMethod)method;

	do {
		if (statuscode >= 301 && statuscode <= 308) {
			char newurl[4096];
			httpcGetResponseHeader(&context, (char*)"Location", &newurl[0], 4096);
			url = &newurl[0];

			httpcCloseContext(&context);
		}

		Result ret = httpcOpenContext(&context, useMethod, (char*)url, 0);
		
		// Just disable SSL verification instead of loading default certs.
		httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		if(headers != NULL){
			char *tokenheader = (char*)malloc(strlen(headers)+1);
			strcpy(tokenheader, headers);
			char *toker = tokenheader;
			char *headername = NULL;
			char *headervalue = NULL;
			do {
				headername = strtok(toker, ":");
				if (headername == NULL) break;
				headervalue = strtok(NULL, "\n");
				if (headervalue == NULL) break;
				if (headervalue[0] == ' ') headervalue++;
				httpcAddRequestHeaderField(&context, headername, headervalue);
				toker = NULL;
			} while (headername != NULL && headervalue != NULL);
			free(tokenheader);
		}

		if (useMethod == HTTPC_METHOD_POST && postdata != NULL) {
			httpcAddPostDataRaw(&context, (u32*)postdata, strlen(postdata));
		}

		#ifndef SKIP_ERROR_HANDLING
		if(ret==0){
		#endif
			httpcBeginRequest(&context);
			u32 contentsize=0;
			httpcGetResponseStatusCode(&context, &statuscode, 0);
			if (statuscode == 200){
				u32 readSize = 0;
				long int bytesWritten = 0;
				u8* buf = (u8*)malloc(0x1000);
				memset(buf, 0, 0x1000);

				Handle fileHandle;
				FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
				FS_Path filePath=fsMakePath(PATH_ASCII, file);
				FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);

				do {
					ret = httpcDownloadData(&context, buf, 0x1000, &readSize);
					FSFILE_Write(fileHandle, NULL, bytesWritten, buf, readSize, 0x10001);
					bytesWritten += readSize;
				} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

				FSFILE_Close(fileHandle);
				svcCloseHandle(fileHandle);
				free(buf);
			}
		#ifndef SKIP_ERROR_HANDLING
		}
		#endif
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));
	#ifndef SKIP_ERROR_HANDLING
	if ((statuscode < 200 && statuscode > 226) && statuscode != 304) luaL_error(L, "error opening url");
	#endif
	httpcCloseContext(&context);
	lua_pushinteger(L, statuscode);
	return 1;
}

static int lua_downstring(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc < 1 || argc > 4) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* url = luaL_checkstring(L,1);
	const char* headers = (argc >= 2) ? luaL_checkstring(L,2) : NULL;
	u8 method = (argc >= 3) ? luaL_checkinteger(L,3) : 0;
	const char* postdata = (argc >= 4) ? luaL_checkstring(L,4) : NULL;
	httpcContext context;
	HTTPC_RequestMethod useMethod = HTTPC_METHOD_GET;

	if(method <= 3 && method >= 1) useMethod = (HTTPC_RequestMethod)method;

	u32 statuscode=0;
	do {
		if (statuscode >= 301 && statuscode <= 308) {
			char newurl[4096];
			httpcGetResponseHeader(&context, (char*)"Location", &newurl[0], 4096);
			url = &newurl[0];

			httpcCloseContext(&context);
		}

		Result ret = httpcOpenContext(&context, useMethod, (char*)url , 0);
		
		// Lets just disable SSL verification instead of loading default certs.
		httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		if(headers != NULL){
			char *tokenheader = (char*)malloc(strlen(headers)+1);
			strcpy(tokenheader, headers);
			char *toker = tokenheader;
			char *headername = NULL;
			char *headervalue = NULL;
			do {
				headername = strtok(toker, ":");
				if (headername == NULL) break;
				headervalue = strtok(NULL, "\n");
				if (headervalue == NULL) break;
				if (headervalue[0] == ' ') headervalue++;
				httpcAddRequestHeaderField(&context, headername, headervalue);
				toker = NULL;
			} while (headername != NULL && headervalue != NULL);
			free(tokenheader);
		}

		if (useMethod == HTTPC_METHOD_POST && postdata != NULL) {
			httpcAddPostDataRaw(&context, (u32*)postdata, strlen(postdata));
		}

		#ifndef SKIP_ERROR_HANDLING
		if(ret==0){
		#endif
			httpcBeginRequest(&context);
			long int contentsize=0; // Crash on the += if u32. WTF?
			u32 readSize=0;
			httpcGetResponseStatusCode(&context, &statuscode, 0);
			if (statuscode == 200) {
				unsigned char *buffer = (unsigned char*)malloc(0x1000);
				do {
					ret = httpcDownloadData(&context, buffer+contentsize, 0x1000, &readSize);
					contentsize += readSize;
					if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
						buffer = (unsigned char*)realloc(buffer, contentsize + 0x1000);
				} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);
				buffer = (unsigned char*)realloc(buffer, contentsize + 1);
				buffer[contentsize] = 0;
				lua_pushlstring(L,(const char*)buffer,contentsize);
				free(buffer);
			}
		#ifndef SKIP_ERROR_HANDLING
		}
		#endif
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));
	#ifndef SKIP_ERROR_HANDLING
	if ((statuscode < 200 && statuscode > 226) && statuscode != 304) luaL_error(L, "error opening url");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3) return luaL_error(L, "wrong number of arguments");
	#endif
	char* to = (char*)luaL_checkstring(L,1);
	char* subj = (char*)luaL_checkstring(L,2);
	char* mex = (char*)luaL_checkstring(L,3);
	int req_size = 70;
	int nss = SpaceCounter(subj);
	int nsm = SpaceCounter(mex);
	req_size = req_size + (nss<<1) + (nsm<<1) + strlen(subj) + strlen(mex) + strlen(to);
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
	Result ret = httpcOpenContext(&context, HTTPC_METHOD_GET, (char*)url , 0);
	#ifndef SKIP_ERROR_HANDLING
	if(ret==0){
	#endif
		httpcBeginRequest(&context);
		HTTPC_RequestStatus loading;
		httpcGetRequestState(&context, &loading);
		while (loading == HTTPC_STATUS_REQUEST_IN_PROGRESS){
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
	#ifndef SKIP_ERROR_HANDLING
	}else luaL_error(L, "error opening url");
	#endif
	httpcCloseContext(&context);
	return 1;
}

static int lua_wifilevel(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8* wifi_link = (u8*)0x1FF81066;
	lua_pushinteger(L,*wifi_link);
	return 1;
}

static int lua_initSock(lua_State *L)
{
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	
	// Init default socketing
	ftp_init();
	ftp_state = true;
	sprintf(shared_ftp,"Waiting for connection...");
	connfd = -1;
	
	// Init SSL support
	sslcCreateRootCertChain(&RootCertChain_contexthandle);
	
	// Add default certificates
	for (int i=0x7;i<0xC;i++){
		sslcRootCertChainAddDefaultCert(RootCertChain_contexthandle, (SSLC_DefaultRootCert)i, NULL);
	}
	
	return 0;
}

static int lua_createServerSocket(lua_State *L)
{
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1 && argc != 2) return luaL_error(L, "Socket.createServerSocket(port) takes one argument.");
	#endif
	int port = luaL_checkinteger(L, 1);
	int type = IPPROTO_TCP;
	if (argc == 2) type = luaL_checkinteger(L, 2);
	
	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	my_socket->serverSocket = true;

	if (type == IPPROTO_TCP) my_socket->sock = socket(AF_INET, SOCK_STREAM, 0);
	else my_socket->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->sock <= 0) return luaL_error(L, "invalid socket.");
	#endif

	int rcvbuf = 32768;
	setsockopt(my_socket->sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
	struct sockaddr_in addrTo;
	addrTo.sin_family = AF_INET;
	addrTo.sin_port = htons(port);
	if (type == IPPROTO_TCP) addrTo.sin_addr.s_addr = 0;
	else addrTo.sin_addr.s_addr = gethostid();

	int err = bind(my_socket->sock, (struct sockaddr*)&addrTo, sizeof(addrTo));
	#ifndef SKIP_ERROR_HANDLING
	if (err != 0) return luaL_error(L, "bind error.");
	#endif

	fcntl(my_socket->sock, F_SETFL, O_NONBLOCK);
	
	if (type == IPPROTO_TCP){
		err = listen(my_socket->sock, 1);
		#ifndef SKIP_ERROR_HANDLING
		if (err != 0) return luaL_error(L, "listen error.");
		#endif
	}
	
	my_socket->magic = 0xDEADDEAD;
	lua_pushinteger(L,(u32)my_socket);
	return 1;
}

static int lua_shutSock(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	
	// Closing default socketing
	ftp_exit();
	ftp_state = false;
	
	// Closing SSL
	sslcDestroyRootCertChain(RootCertChain_contexthandle);
	
	return 0;
}

static int lua_recv(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	
	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	u32 size = luaL_checkinteger(L, 2);
	
	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");			
	if (my_socket->serverSocket) return luaL_error(L, "recv not allowed for server sockets.");
	#endif

	char* data = (char*)malloc(size);
	int count = 0;
	if (my_socket->isSSL) count = sslcRead(&my_socket->sslc_context, data, size, false);
	else count = recv(my_socket->sock, data, size, 0);
	if (count > 0) lua_pushlstring(L, data, count);
	else lua_pushstring(L, "");
	free(data);
	return 1;
}

static int lua_send(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING	
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	
	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	size_t size;
	char* text = (char*)luaL_checklstring(L, 2, &size);

	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	if (my_socket->serverSocket) return luaL_error(L, "send not allowed for server sockets.");
	if (!text) return luaL_error(L, "Socket.send() expected a string.");
	#endif
	
	int result = 0;
	if (my_socket->isSSL) result = sslcWrite(&my_socket->sslc_context, text, size);
	else result = sendto(my_socket->sock, text, size, 0, NULL, 0);
	lua_pushinteger(L, result);
	return 1;
}

static int lua_connect(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2 && argc != 4 && argc != 3)  return luaL_error(L, "wrong number of arguments");
	#endif
	
	// Getting arguments
	char *host = (char*)luaL_checkstring(L, 1);
	int port = luaL_checkinteger(L, 2);
	bool isSSL = false;
	if (argc >= 3) isSSL = lua_toboolean(L,3);
	int type = IPPROTO_TCP;
	if (argc == 4) type = luaL_checkinteger(L, 4);
	char port_str[8];
	sprintf(port_str,"%i",port);
	
	// Allocating Socket memblock
	Socket* my_socket = (Socket*) malloc(sizeof(Socket));
	my_socket->serverSocket = false;
	my_socket->magic = 0xDEADDEAD;
	my_socket->isSSL = isSSL;
	
	// Creating socket
	if (type == IPPROTO_TCP) my_socket->sock = socket(AF_INET, SOCK_STREAM, 0);
	else my_socket->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->sock < 0){
		free(my_socket);
		return luaL_error(L, "Failed creating socket.");
	}
	#endif
	
	// Resolving host
	struct addrinfo hints;
	struct addrinfo *resaddr = NULL, *resaddr_cur;	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(host, port_str, &hints, &resaddr);
	
	// Connecting to the server
	for(resaddr_cur = resaddr; resaddr_cur!=NULL; resaddr_cur = resaddr_cur->ai_next){
		if(connect(my_socket->sock, resaddr_cur->ai_addr, resaddr_cur->ai_addrlen)==0)break;
	}
	
	freeaddrinfo(resaddr);
	#ifndef SKIP_ERROR_HANDLING
	if(resaddr_cur==NULL){
		closesocket(my_socket->sock);
		free(my_socket);
		return luaL_error(L, "Failed connecting to the server.");
	}
	#endif
	
	// Opening SSL connection
	if (isSSL){
		Result ret = sslcCreateContext(&my_socket->sslc_context, my_socket->sock, SSLCOPT_DisableVerify, host);
		#ifndef SKIP_ERROR_HANDLING
		if(R_FAILED(ret)){
			closesocket(my_socket->sock);
			free(my_socket);
			return luaL_error(L, "Failed creating SSL context.");
		}
		#endif
		sslcContextSetRootCertChain(&my_socket->sslc_context, RootCertChain_contexthandle);
		ret = sslcStartConnection(&my_socket->sslc_context, NULL, NULL);
		#ifndef SKIP_ERROR_HANDLING
		if(R_FAILED(ret)){
			closesocket(my_socket->sock);
			free(my_socket);
			return luaL_error(L, "SSL connection failed.");
		}
		#endif
	}
	
	// Setting socket options
	int sock_buffersize = 32768;
	setsockopt(my_socket->sock, SOL_SOCKET, SO_RCVBUF, &sock_buffersize, sizeof(sock_buffersize));
	int flags = fcntl(my_socket->sock, F_GETFL, 0);
	fcntl(my_socket->sock, F_SETFL, flags | O_NONBLOCK);
	
	lua_pushinteger(L, (u32)my_socket);
	return 1;
}

static int lua_addCert(lua_State *L)
{
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1)  return luaL_error(L, "wrong number of arguments");
	#endif
	const char *text = luaL_checkstring(L, 1);
	fileStream fileHandle;
	if (strncmp("romfs:/",text,7) == 0){
		fileHandle.isRomfs = true;
		FILE* handle = fopen(text,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle.handle = (u32)handle;
	}else{
		fileHandle.isRomfs = false;
		FS_Path filePath = fsMakePath(PATH_ASCII, text);
		FS_Archive script=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		Result ret = FSUSER_OpenFileDirectly( &fileHandle.handle, script, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "file doesn't exist.");
		#endif
	}
	u64 cert_size;
	u32 bytesRead;
	FS_GetSize(&fileHandle, &cert_size);
	u8* cert = (u8*)malloc(cert_size);
	FS_Read(&fileHandle, &bytesRead, 0, cert, cert_size);
	sslcAddTrustedRootCA(RootCertChain_contexthandle, cert, cert_size, NULL);
	free(cert);
	FS_Close(&fileHandle);
	return 0;
}

static int lua_accept(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	
	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	
	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	if (!my_socket->serverSocket) return luaL_error(L, "accept allowed for server sockets only.");
	#endif

	struct sockaddr_in addrAccept;
	socklen_t cbAddrAccept = sizeof(addrAccept);
	int sockClient = accept(my_socket->sock, (struct sockaddr*)&addrAccept, &cbAddrAccept);
	if (sockClient <= 0) return 0;

	Socket* incomingSocket = (Socket*) malloc(sizeof(Socket));
	incomingSocket->serverSocket = 0;
	incomingSocket->sock = sockClient;
	incomingSocket->magic = 0xDEADDEAD;
	int rcvbuf = 32768;
	setsockopt(incomingSocket->sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
	lua_pushinteger(L, (u32)incomingSocket);
	return 1;
}

static int lua_closeSock(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "Socket.close() takes one argument.");
	#endif
	
	Socket* my_socket = (Socket*)luaL_checkinteger(L, 1);
	
	#ifndef SKIP_ERROR_HANDLING
	if (my_socket->magic != 0xDEADDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	if (my_socket->isSSL) sslcDestroyContext(&my_socket->sslc_context);
	closesocket(my_socket->sock);
	free(my_socket);
	return 0;
}

//Register our Network Functions
static const luaL_Reg Network_functions[] = {
  {"updateFTP",     lua_checkFTPcommand},
  {"isWifiEnabled", lua_wifistat},
  {"getWifiLevel",  lua_wifilevel},
  {"getMacAddress", lua_macaddr},
  {"getIPAddress",  lua_ipaddr},
  {"downloadFile",  lua_download},
  {"requestString", lua_downstring},
  {"sendMail",      lua_sendmail},
  {"addCertificate",lua_addCert},
  {0, 0}
};

//Register our Socket Functions
static const luaL_Reg Socket_functions[] = {
  {"init",              lua_initSock},
  {"term",              lua_shutSock},
  {"createServerSocket",lua_createServerSocket},
  {"connect",           lua_connect},
  {"receive",           lua_recv},
  {"send",              lua_send},
  {"accept",            lua_accept},
  {"close",             lua_closeSock},
  {0, 0}
};

void luaNetwork_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Network_functions, 0);
	lua_setglobal(L, "Network");
	lua_newtable(L);
	luaL_setfuncs(L, Socket_functions, 0);
	lua_setglobal(L, "Socket");
	u8 GET_METHOD = 1;
	u8 POST_METHOD = 2;
	u8 HEAD_METHOD = 3;
	VariableRegister(L,IPPROTO_UDP);
	VariableRegister(L,IPPROTO_TCP);
	VariableRegister(L,POST_METHOD);
	VariableRegister(L,GET_METHOD);
	VariableRegister(L,HEAD_METHOD);
}
