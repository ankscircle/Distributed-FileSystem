/*
FUSE: Filesystem in Userspace
Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

This program can be distributed under the terms of the GNU GPL.
See the file COPYING.

gcc -Wall `pkg-config fuse --cflags --libs` fusexmp.c -o fusexmp
 */

#define FUSE_USE_VERSION 26


#include <stdlib.h>
#include <fuse.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <ctype.h>
#include <libgen.h>
#include <limits.h>
#include<stdarg.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif


int portno_c,sockfd;
char hostname[100];
struct data_sent
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
}My;

int auth_completed;
int loop;
struct data_received
{
	int res;
	int filefd;
	struct stat stbuf;
	uint64_t dp;
	uint64_t dirdp;
};


int connect_server(void);


static int xmp_getattr(const char *path, struct stat *stbuf)
{
	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"getattr");

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);
	memcpy(stbuf,&response->stbuf,sizeof(struct stat));

	close(socket_fd);
	return response->res;
}

static int xmp_access(const char *path, int mask)
{

	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"access");
	request->mask= mask;
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);
	n=recv(socket_fd,response,sizeof(struct data_received),0);
	close(socket_fd);
	return response->res;
}

static int xmp_readlink(const char *path, char *buf, size_t size)
{
	//logmsg("READLINK\n");
	int res;

	res = readlink(path, buf, size - 1);
	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a',token=1;
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"readdir");

	request->dir = fi->fh;

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);

	if(response->res!=0)
		return response->res;

	sync = 'a';
	if(response->res==0)
	{
		while(token !=0)
		{
			char name[100];
			int filler_c=0;

			n=send(socket_fd,&sync,sizeof(char),0);

			n=recv(socket_fd,&token,sizeof(int),0);

			if(token==0)
				break;
			sync = 'a';
			n=send(socket_fd,&sync,sizeof(char),0);

			n=recv(socket_fd,&name,sizeof(name),0);

			if (filler(buf, name, NULL, 0))
			{
				filler_c=1;
			}
			n=send(socket_fd,&filler_c,sizeof(int),0);
			if(filler_c==1)
				break;
		}
	}

	close(socket_fd);
	return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev)
{
	//logmsg("MKNOD\n");
	int res;

	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) {
		res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISFIFO(mode))
		res = mkfifo(path, mode);
	else
		res = mknod(path, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_mkdir(const char *path, mode_t mode)
{


	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"mkdir");
	request->mode= mode;
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);
	close(socket_fd);
	return response->res;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{


	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"create");
	request->mode= mode;

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);
	fi->fh=response->filefd;
	close(socket_fd);
	return response->res;
}

static int xmp_unlink(const char *path)
{

	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"unlink");

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);

	close(socket_fd);
	return response->res;


}

static int xmp_rmdir(const char *path)
{


	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"rmdir");

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);

	close(socket_fd);
	return response->res;

}

static int xmp_symlink(const char *from, const char *to)
{
	//logmsg("SYMLINK\n");
	int res;

	res = symlink(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_rename(const char *from, const char *to)
{
	//logmsg("RENAME\n");
	int res;

	res = rename(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_link(const char *from, const char *to)
{
	//logmsg("LINK\n");
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chmod(const char *path, mode_t mode)
{
	//logmsg("CHMOD\n");
	int res;

	res = chmod(path, mode);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid)
{
	///logmsg("CHOWN\n");
	int res;

	res = lchown(path, uid, gid);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_truncate(const char *path, off_t size)
{
	//logmsg("TRUNCATE\n");
	int res;

	res = truncate(path, size);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_utimens(const char *path, const struct timespec ts[2])
{
	//logmsg("UTIMENS\n");
	int res;
	struct timeval tv[2];

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);
	if (res == -1)
		return -errno;

	return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{

	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"open");
	request->flags=fi->flags;
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);

	fi->fh=response->filefd;

	close(socket_fd);
	return response->res;

}
static int xmp_opendir(const char *path, struct fuse_file_info *fi)
{
	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"opendir");

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);

	n=recv(socket_fd,response,sizeof(struct data_received),0);

	if(response->res==0)
		fi->fh = response->dp;
	close(socket_fd);
	return response->res;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	(void)fi;

	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"read");
	request->filefd=fi->fh;
	request->size=size;
	request->offset=offset;
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	n=recv(socket_fd,response,sizeof(struct data_received),0);

	if(response->res<0)
		return response->res;

	if(response->res>0)
	{
		size_t len;
		sync='a';
		n=send(socket_fd,&sync,sizeof(char),0);


		n=recv(socket_fd,buf,15000,0);
	}
	close(socket_fd);
	return response->res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
		off_t offset, struct fuse_file_info *fi)
{
	char rbuf[4096];

	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"write");


	strcpy(rbuf,buf);
	request->filefd=fi->fh;
	request->offset=offset;
	request->size=size;


	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);



	sync='a';
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,&rbuf,sizeof(rbuf),0);

	n=recv(socket_fd,response,sizeof(struct data_received),0);
	close(socket_fd);
	return response->res;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf)
{
	//logmsg("STATFS\n");
	int res;

	res = statvfs(path, stbuf);
	if (res == -1)
		return -errno;

	return 0;
}
static int xmp_releasedir(const char *path, struct fuse_file_info *fi)
{


	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));

	(void) path;
	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"releasedir");
	request->dir=fi->fh;
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);

	sync='a';
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}
	close(socket_fd);
	return 0;
}

static int xmp_release(const char *path, struct fuse_file_info *fi)
{


	struct data_sent *request=(struct data_sent *) malloc(sizeof(struct data_sent));
	struct data_received *response=(struct data_received *) malloc(sizeof(struct data_received));

	memset(request,0,sizeof(struct data_sent));
	memset(response,0,sizeof(struct data_received));


	char sync='a';
	int socket_fd,n;
	socket_fd = connect_server();

	strcpy(request->path,path);
	strcpy(request->func,"release");
	request->filefd= fi->fh;

	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}

	n=send(socket_fd,request,sizeof(struct data_sent),0);


	sync='a';
	while(sync!='s')
	{
		n=recv(socket_fd,&sync,sizeof(char),0);
	}
	close(socket_fd);

	return 0;
}

static int xmp_fsync(const char *path, int isdatasync,struct fuse_file_info *fi)
{
	//logmsg("FSYNC\n");
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int xmp_setxattr(const char *path, const char *name, const char *value,
		size_t size, int flags)
{
	//logmsg("SETXATTR\n");
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int xmp_getxattr(const char *path, const char *name, char *value,
		size_t size)
{
	//logmsg("GETxATTR\n");
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_listxattr(const char *path, char *list, size_t size)
{
	//logmsg("LISTXATTR\n");
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int xmp_removexattr(const char *path, const char *name)
{
	//logmsg("REMOVEATTR\n");
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

int connect_server(void)
{
	int n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);


	server = gethostbyname(hostname);
	if (server == NULL) {
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr,server->h_addr_list[0],server->h_length);
	serv_addr.sin_port = htons(portno_c);
	while(connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr))<0);
start_prof:

	if(auth_completed==1)
	{
		n=send(sockfd,&My,sizeof(struct profile),0);
	}
	else
	{

		printf("Are you registered?\n ENTER 1: Yes  ENTER 0: No\n Option: ");
		scanf("%d",&My.reg);
		if(My.reg==1)
		{
			printf("Username: ");
			scanf("%s",&My.username);
			printf("Password: ");
			scanf("%s",&My.password);
			strcat(My.password,"\n");
			n=send(sockfd,&My,sizeof(struct profile),0);
			n=recv(sockfd,&loop,sizeof(int),0);
			if(loop==1)
			{
				goto start_prof;
			}
			else
			{
				auth_completed=1;
				My.reg=2;
			}

		}
		else if(My.reg==0)
		{
			printf("Enter New Username: ");
			scanf("%s",My.username);
			printf("Password: ");
			scanf("%s",My.password);
			strcat(My.password,"\n");
			n=send(sockfd,&My,sizeof(struct profile),0);
			My.reg=2;
			auth_completed=1;
		}
	}
	return sockfd;

}
static struct fuse_operations xmp_oper = {
	.getattr	= xmp_getattr,
	.create		= xmp_create,
	.access		= xmp_access,
	//.readlink	= xmp_readlink,
	.opendir	= xmp_opendir,
	.readdir	= xmp_readdir,
	//.mknod		= xmp_mknod,
	.mkdir		= xmp_mkdir,
	//.symlink	= xmp_symlink,
	.unlink		= xmp_unlink,
	.rmdir		= xmp_rmdir,
	//.rename		= xmp_rename,
	//.link		= xmp_link,
	/*.chmod		= xmp_chmod,
	  .chown		= xmp_chown,
	  .truncate	= xmp_truncate,
	  .utimens	= xmp_utimens,*/
	.open		= xmp_open,
	.read		= xmp_read,
	.write		= xmp_write,
	//.statfs		= xmp_statfs,
	.release	= xmp_release,
	.releasedir	=xmp_releasedir,
	/*.fsync		= xmp_fsync,
#ifdef HAVE_SETXATTR
.setxattr	= xmp_setxattr,
.getxattr	= xmp_getxattr,
.listxattr	= xmp_listxattr,
.removexattr	= xmp_removexattr,
#endif*/
};

int main(int argc, char *argv[])
{
	//	umask(0);
	auth_completed==0;
	loop=0;

	portno_c=atoi(argv[1]);
	strcpy(hostname,argv[2]);
	int sockfd= connect_server();
	if(argc<3)
	{
		exit(0);
	}

	int i;
	for(i=1;i<argc-2;i++)
	{
		argv[i]=argv[i+2];
	}
	argc--;
	argc--;

	return fuse_main(argc, argv, &xmp_oper, NULL);
}

