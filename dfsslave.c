#define FUSE_USE_VERSION 25
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/xattr.h>
#include<sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include<time.h>
#include<stdarg.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include<errno.h>
#define CLIENTS 300
int sockfd;
static const char realpat[100];// ="/home/apawar2/server1";
int filefd;
char fpath[200];
int res;
struct data_received{
char path[200];
char func[100];
mode_t mode;
int filefd;
int mask;
size_t size;
off_t offset;
char buf[4096];
int flags;
uint64_t dir;
};




int sockf;
struct sockaddr_in fserver;
int newsockfd;

int main(int argc, char *argv[])
{
long sizel;
char *serverbuf;
char *ptr;


sizel = pathconf(".", _PC_PATH_MAX);


if ((serverbuf = (char *)malloc((size_t)sizel)) != NULL)
    ptr = getcwd(serverbuf, (size_t)sizel);

strcpy(realpat,serverbuf);
     socklen_t clilen;
          struct sockaddr_in serv_addr, cli_addr;
     int n;


struct data_received * request= (struct data_received *) malloc(sizeof(struct data_received));
 sockfd = socket(AF_INET, SOCK_STREAM, 0);

  bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;

    serv_addr.sin_port = htons(atoi(argv[1]));

     if (bind(sockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) >= 0)
{

clilen=sizeof(cli_addr);
     listen(sockfd,6);
 while(1)
{
int ret;
start_up:
newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);


memset(request,0,sizeof(struct data_received));



char sync='s';
n = send(newsockfd,&sync,sizeof(char),0);


n = recv(newsockfd,request,sizeof(struct data_received),0);

strcpy(fpath,realpat);
strcat(fpath,request->path);

printf("%s\n",request->func);

if(!strcmp(request->func,"mkdir"))
{

printf(" Inside: %s", fpath);
res = mkdir(fpath, request->mode);
        if (res == -1)
res=-errno;



}

if(!strcmp(request->func,"rmdir"))
{
printf(" Inside: %s", fpath);
res = rmdir(fpath);
                if (res < -1)
res=-errno;


}

if(!strcmp(request->func,"unlink"))
{
printf(" Inside: %s", fpath);
res = unlink(fpath);
                if (res < 0)
                res=-errno;



}
if(!strcmp(request->func,"write"))
{
char bufw[4096];
printf(" Inside: %s", fpath);
int filefd= open(fpath,O_WRONLY);

char syncw='s';
n=send(newsockfd,&syncw,sizeof(char),0);

n=recv(newsockfd,&bufw,sizeof(bufw),0);


res = pwrite(filefd, bufw, request->size, request->offset);

    if (res < 0)
        res =-errno;


}
if(!strcmp(request->func,"create"))
{

filefd = creat(fpath, request->mode);
        if (filefd<0)
res=-errno;

}

close(newsockfd);
}
}
return 0;
}
