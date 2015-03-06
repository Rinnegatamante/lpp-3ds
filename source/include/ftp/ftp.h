#ifndef FTP_H
#define FTP_H

extern FS_archive sdmcArchive;
extern char currentPath[];
extern u32 currentIP;
extern int dataPort;
extern char shared_ftp[256];

void ftp_init();
void ftp_exit();

int ftp_frame(int s);
int ftp_getConnection();

int ftp_openCommandChannel();
int ftp_openDataChannel();
int ftp_sendResponse(int s, int n, char* mes);

#endif
