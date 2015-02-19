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
#- Smealum for ctrulib -------------------------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"

static int lua_wifistat(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u32 wifiStatus;
	ACU_GetWifiStatus(NULL, &wifiStatus);
	lua_pushboolean(L,wifiStatus);
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

static int lua_download(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char* url = luaL_checkstring(L,1);
	const char* file = luaL_checkstring(L,2);
	httpcContext context;
	Result ret = httpcOpenContext(&context, (char*)url , 0);
	if(ret==0){
		httpcBeginRequest(&context);
		/*httpcReqStatus loading;  <-- Seems to freeze on CIA build
		httpcGetRequestState(&context, &loading);
		while (loading != 0x7){
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
		/*httpcReqStatus loading; <-- Seems to freeze on CIA build
		httpcGetRequestState(&context, &loading);
		while (loading != 0x7){
			httpcGetRequestState(&context, &loading);
		}*/
		u32 statuscode=0;
		u32 contentsize=0;
		httpcGetResponseStatusCode(&context, &statuscode, 0);
		if (statuscode != 200) luaL_error(L, "request error");
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
		while (loading != 0x7){
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

//Register our Network Functions
static const luaL_Reg Network_functions[] = {
  {"isWifiEnabled",			lua_wifistat},
  {"getMacAddress",			lua_macaddr},
  {"downloadFile",			lua_download},
  {"requestString",			lua_downstring},
  {"sendMail",				lua_sendmail},
  {0, 0}
};

void luaNetwork_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Network_functions, 0);
	lua_setglobal(L, "Network");
}