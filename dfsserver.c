#define FUSE_USE_VERSION 26
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
#include <sys/stat.h>
#include<time.h>
#include<stdarg.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include<errno.h>
#include<fuse.h>
#define CLIENTS 300
//#define bzero(b,len) (memset((char *)(b), '\0', (len)), (void) 0)
#define log_struct(st, field, format, typecast) \
	log_msg("    " #field " = " #format "\n", typecast st->field)

#define _USE_STD_STAT
#define MAX_CLIENTS 40
int sockfd;
static const char serverpat[100];// ="/home/apawar2/server";

char fpath[200];
char slave[100];
int slaveport;
char wrtbuf[4096];
int slavefd;

struct data_received
{
	char path[200];
	char func[100];
	mode_t mode;
	int filefd;
	int mask;
	size_t size;
	off_t offset;
	int flags;
	uint64_t dir;
};


struct profile
{
	char username[100];
	char password[100];
	int reg;
}profile_l[MAX_CLIENTS],present_user,client_reg;

struct data_sent
{
	int res;
	int filefd;
	struct stat stbuf;
	intptr_t dp;
	uint64_t dirdp;
};

int validation_client(void);
int make_socket(int a, struct sockaddr_in b);
int sockf;
struct sockaddr_in fserver;
int newsockfd;
int present_max;
char realpat[200];

void fill_profiles(void)
{
	char k[100];
	strcpy(k,serverpat);
	strcat(k,"/.logging");
	FILE *filefd;
	filefd= fopen(k,"r");
	char pick[100];
	int i=0;

	if(filefd!=NULL)
	{
		while(fgets(pick,sizeof(pick),filefd)!=NULL)
		{
			char *pick_p = malloc(100);
			char *delimit=malloc(100);
			strcpy(pick_p,pick);
			delimit=strtok(pick_p,"|");
			if(delimit!=NULL)
				strcpy(profile_l[i].username,delimit);
			delimit=strtok(NULL,"|");
			if(delimit!=NULL)
				strcpy(profile_l[i].password,delimit);
			i++;
		}
		present_max=i;
		for(i=0;i<present_max;i++)
			fclose(filefd);
	}
	else
	{
		perror("FILE: ");
	}

}

int validation_client(void)
{

	int ret;
	int n,i;
	int loop=1;
start_prof:

	n=recv(newsockfd,&client_reg,sizeof(struct profile),0);
	if(client_reg.reg==1)
	{
		while(loop==1)
		{
			for(i=0;i<present_max;i++)
				if(strcmp(client_reg.username,profile_l[i].username)==0 && strcmp(client_reg.password,profile_l[i].password)==0)
				{
					loop=0;
					printf("Authenticated client\n");
					n=send(newsockfd,&loop,sizeof(int),0);
					break;
				}
			if(loop==1)
			{
				printf("Authentication failed\n");
				n=send(newsockfd,&loop,sizeof(int),0);
				goto start_prof;
			}

		}
		ret=0;
	}
	else if(client_reg.reg==0)
	{
		char insert[200];
		present_max++;
		strcpy(profile_l[present_max].username,client_reg.username);
		strcpy(profile_l[present_max].password,client_reg.password);
		strcpy(insert,client_reg.username);
		strcat(insert,"|");
		strcat(insert,client_reg.password);
		FILE *inst;
		FILE *filefd;
		char k[100];
		strcpy(k,serverpat);
		strcat(k,"/.logging");
		inst= fopen(k,"a");
		fputs(insert,inst);
		fclose(inst);
		printf("Registered client\n");
		int dircheck;

		strcpy(realpat,serverpat);
		strcpy(fpath,realpat);
		strcat(fpath,"/");
		strcat(fpath,client_reg.username);
		dircheck=mkdir(fpath,S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);

		if(dircheck<0)
			dircheck= -1;
		struct data_received * request= (struct data_received *) malloc(sizeof(struct data_received));

		struct sockaddr_in slave_addr;
		struct hostent *slaveent;
		slavefd = socket(AF_INET, SOCK_STREAM, 0);
		slaveent = gethostbyname(slave);
		if (slaveent== NULL) {
			exit(0);
		}
		bzero((char *) &slave_addr, sizeof(slave_addr));
		slave_addr.sin_family = AF_INET;
		memcpy(&slave_addr.sin_addr,slaveent->h_addr_list[0],slaveent->h_length);
		slave_addr.sin_port = htons(slaveport);
		while(connect(slavefd,(struct sockaddr *) &slave_addr,sizeof(slave_addr))<0);

		char temppath[200];
		strcpy(temppath,"/");
		strcat(temppath,client_reg.username);
		strcpy(request->func,"mkdir");
		request->mode=S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH;
		strcpy(request->path,temppath);
		n=send(slavefd,request,sizeof(struct data_received),0);
		close(slavefd);
		ret=0;
	}
	else if(client_reg.reg==2)
	{
		strcpy(realpat,serverpat);
		strcat(realpat,"/");
		strcat(realpat,client_reg.username);
		present_user=client_reg;
		ret=1;
	}
	return ret;
}

/*	THE MAIN FUNCTION 	*/

int main(int argc, char *argv[])
{
	long size;
	char *serverbuf;
	char *ptr;
	size = pathconf(".", _PC_PATH_MAX);

	if ((serverbuf = (char *)malloc((size_t)size)) != NULL)
		ptr = getcwd(serverbuf, (size_t)size);

	strcpy(serverpat,serverbuf);

	if(argc<4)
	{
		printf(" USAGE: [Master_port] [Slave_port] [Slave_Hostname]\n");
		exit(0);
	}
	strcpy(slave,argv[3]);
	slaveport=atoi(argv[2]);
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	fill_profiles();

	struct data_received * request= (struct data_received *) malloc(sizeof(struct data_received));
	struct data_sent *response= (struct data_sent *) malloc(sizeof(struct data_sent));
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
			ret = validation_client();

			if(ret==0)
			{
				close(newsockfd);
				goto start_up;
			}

			memset(request,0,sizeof(struct data_received));
			memset(response,0,sizeof(struct data_sent));

			char sync='s';
			n = send(newsockfd,&sync,sizeof(char),0);
			n = recv(newsockfd,request,sizeof(struct data_received),0);

			strcpy(fpath,realpat);
			strcat(fpath,request->path);

			if(!strcmp(request->func,"getattr"))
			{
				char sync='s';
				struct stat *temp = (struct stat *)malloc(sizeof(struct stat));

				response->res = lstat(fpath, temp);
				memcpy(&response->stbuf,temp,sizeof(struct stat));
				if(response->res<0)
					response->res=-errno;
				n = send(newsockfd,response,sizeof(struct data_sent),0);
			}
			if(!strcmp(request->func,"readdir"))
			{
				DIR *dp;
				struct dirent *de;
				char sync_dir;
				int token=1;
				int filler_c=0;
				dp = (DIR *) (uintptr_t) request->dir;
				de = readdir(dp);

				if(de==NULL)
				{
					response->res=-errno;
					n = send(newsockfd,response,sizeof(struct data_sent),0);
					break;
				}
				if(response->res==0)
				{
					n = send(newsockfd,response,sizeof(struct data_sent),0);
					if(de!=NULL)
						do
						{
							char name[100];
							strcpy(name,de->d_name);
							sync_dir='r';

							while(sync_dir!='a')
							{
								n = recv(newsockfd,&sync_dir,sizeof(char),0);
							}

							n = send(newsockfd,&token,sizeof(int),0);
							sync_dir='r';
							while(sync_dir!='a')
							{
								n = recv(newsockfd,&sync_dir,sizeof(char),0);
							}

							n = send(newsockfd,&name,sizeof(name),0);
							n = recv(newsockfd,&filler_c,sizeof(int),0);

							if(filler_c==1)
								break;

						}while ((de = readdir(dp)) != NULL) ;

					sync_dir='r';
					while(sync_dir!='a')
					{
						n = recv(newsockfd,&sync_dir,sizeof(char),0);
					}
					token=0;
					n = send(newsockfd,&token,sizeof(int),0);
				}
			}


			if(!strcmp(request->func,"mkdir"))
			{
				char sync='s';

				response->res = mkdir(fpath, request->mode);
				if (response->res == -1)
					response->res=-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);
			}
			if(!strcmp(request->func,"access"))
			{

				response->res = access(fpath,request->mask);
				if (response->res == -1)
					response->res=-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);
			}

			if(!strcmp(request->func,"rmdir"))
			{

				response->res = rmdir(fpath);
				if (response->res < -1)
					response->res=-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);
			}

			if(!strcmp(request->func,"unlink"))
			{
				response->res = unlink(fpath);
				if (response->res < 0)
					response->res=-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);
			}
			if(!strcmp(request->func,"opendir"))
			{
				DIR *dp;
				char sync='s';

				dp = opendir(fpath);
				if (dp == NULL)
					response->res=-errno;
				response->dp = (intptr_t)dp;
				n = send(newsockfd,response,sizeof(struct data_sent),0);


			}

			if(!strcmp(request->func,"open"))
			{

				response->filefd = open(fpath, request->flags);
				if (response->filefd < 0)
					response->res =-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);

			}
			if(!strcmp(request->func,"releasedir"))
			{
				sync='s';

				closedir((DIR *) (uintptr_t) request->dir);

				n = send(newsockfd,&sync,sizeof(char),0);
			}

			if(!strcmp(request->func,"release"))
			{
				close(request->filefd);

				n = send(newsockfd,&sync,sizeof(char),0);
			}

			if(!strcmp(request->func,"read"))
			{
				char sync;
				char *buf=(char *)malloc(15000);
				response->res = pread(request->filefd, buf, request->size, request->offset);

				if (response->res < 0)
					response->res =-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);
				if(response->res)
				{
					size_t len;
					sync='s';
					while(sync!='a')
						n=recv(newsockfd,&sync,sizeof(char),0);

					n=send(newsockfd,buf,sizeof(buf),0);
				}


			}

			if(!strcmp(request->func,"write"))
			{
				char sync='s';
				n = send(newsockfd,&sync,sizeof(char),0);
				n = send(newsockfd,&wrtbuf,sizeof(wrtbuf),0);


				response->res = pwrite(request->filefd, wrtbuf, request->size, request->offset);

				if (response->res < 0)
					response->res =-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);

			}
			if(!strcmp(request->func,"create"))
			{

				response->filefd = creat(fpath, request->mode);
				if (response->filefd<0)
					response->res=-errno;

				n = send(newsockfd,response,sizeof(struct data_sent),0);

			}

			close(newsockfd);

			struct sockaddr_in slave_addr;
			struct hostent *slaveent;
			slavefd = socket(AF_INET, SOCK_STREAM, 0);
			slaveent = gethostbyname(slave);
			if (slaveent== NULL) 
			{
				exit(0);
			}

			bzero((char *) &slave_addr, sizeof(slave_addr));
			slave_addr.sin_family = AF_INET;
			memcpy(&slave_addr.sin_addr,slaveent->h_addr_list[0],slaveent->h_length);
			slave_addr.sin_port = htons(slaveport);
			while(connect(slavefd,(struct sockaddr *) &slave_addr,sizeof(slave_addr))<0);

			char temppath[200];
			strcpy(temppath,"/");
			strcat(temppath,present_user.username);
			strcat(temppath,request->path);
			strcpy(request->path,temppath);
			if(!strcmp(request->func,"write"))
			{
				n=send(slavefd,request,sizeof(struct data_received),0);

				char sy='a';
				while(sy!='s')
					n=recv(slavefd,&sy,sizeof(char),0);

				n=send(slavefd,&wrtbuf,sizeof(wrtbuf),0);
			}
			else
			{
				n=send(slavefd,request,sizeof(struct data_received),0);
			}
			close(slavefd);


		}
	}
	return 0;
}
