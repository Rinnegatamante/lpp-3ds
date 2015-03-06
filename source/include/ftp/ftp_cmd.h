#ifndef FTP_CMD_H
#define FTP_CMD_H

#define PATH_EXISTS (0xC82044BE)

typedef void (*ftp_cmdhandler)(int s, char* cmd, char* arg);
typedef struct
{
	char* name;
	ftp_cmdhandler handler;
}ftp_cmd_s;

extern ftp_cmd_s ftp_cmd[];
extern int ftp_cmd_num;

#endif
