#include <stdio.h>
//#include "../../lib/posix/stdio.h"
#include <stdlib.h>
//#include "../../lib/posix/stdlib.h"
#include <unistd.h>
//#include "../../lib/posix/unistd.h"
#include <errno.h>
#include <str.h>
//#include "../../lib/posix/string.h"
#include <fcntl.h>
#include "../../posix/include/posix/signal.h"
//#include<signal.h>
#include <sys/types.h>
#include "../../lib/c/include/net/socket.h"
#include "../../lib/c/include/net/in.h"
#include "../../lib/c/include/net/inet.h"
#include <str_error.h>
#include <assert.h>

/*#include "posix/stdio.h"
#include "posix/stdlib.h"
#include "posix/unistd.h"
//#include <unistd.h>
#include <errno.h>
#include "posix/string.h"
#include <fcntl.h>
#include "posix/signal.h"
#include <sys/types.h>
#include "../../lib/c/include/net/socket.h"
#include "../../lib/c/include/net/in.h"
#include "../../lib/c/include/net/inet.h"*/
#define VERSION 23
#define BUFSIZE 8096
#define ERROR      42
#define LOG        44
#define FORBIDDEN 403
#define NOTFOUND  404
#define SIGCLD SIGCHLD /* Same as SIGCHLD (System V). */
#define SIGCHLD 17 /* Child status has changed (POSIX). */

#define NAME  "nweb"

#define DEFAULT_PORT  8080
#define BACKLOG_SIZE  3

#define WEB_ROOT  "/data/web"

/** Buffer for receiving the request. */
#define BUFFER_SIZE  1024



void logger(FILE *fp,int type, char *s1, char *s2, int socket_fd);
void web(int fd, int hit);
//pid_t setpgrp(void);
//static uint16_t port = DEFAULT_PORT;
static char rbuf[BUFFER_SIZE];
static size_t rbuf_out;
static size_t rbuf_in;

static char lbuf[BUFFER_SIZE + 1];
static size_t lbuf_used;

static char fbuf[BUFFER_SIZE];
static bool verbose = false;
/** Responses to send to client. */

static const char *msg_ok =
    "HTTP/1.0 200 OK\r\n"
    "\r\n";

static const char *msg_bad_request =
    "HTTP/1.0 400 Bad Request\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>400 Bad Request</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Bad Request</h1>\r\n"
    "<p>The requested URL has bad syntax.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";

static const char *msg_not_found =
    "HTTP/1.0 404 Not Found\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>404 Not Found</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Not Found</h1>\r\n"
    "<p>The requested URL was not found on this server.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";

static const char *msg_not_implemented =
    "HTTP/1.0 501 Not Implemented\r\n"
    "\r\n"
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
    "<html><head>\r\n"
    "<title>501 Not Implemented</title>\r\n"
    "</head>\r\n"
    "<body>\r\n"
    "<h1>Not Implemented</h1>\r\n"
    "<p>The requested method is not implemented on this server.</p>\r\n"
    "</body>\r\n"
    "</html>\r\n";
 const char *forbdn =
"HTTP/1.1 403 Forbidden\r\n"
"Content-Length: 185\r\n"
"Connection: close\r\n"
"Content-Type: text/html\n\n"
"<html><head>\r\n"
"<title>403 Forbidden</title>\r\n"
"</head><body>\r\n"
"<h1>Forbidden</h1>\r\n"
"The requested URL, file type or operation is not allowed on this simple static file webserver.\r\n"
"</body>\r\n"
"</html>\r\n";

const char *notfound =
"HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n";




struct {
	char *ext;
	char *filetype;
} extensions [] = {
	{(char *)"gif", (char *)"image/gif" },  
	{(char *)"jpg", (char *)"image/jpg" }, 
	{(char *)"jpeg",(char *)"image/jpeg"},
	{(char *)"png", (char *)"image/png" },  
	{(char *)"ico", (char *)"image/ico" },  
	{(char *)"zip", (char *)"image/zip" },  
	{(char *)"gz",  (char *)"image/gz"  },  
	{(char *)"tar", (char *)"image/tar" },  
	{(char *)"htm", (char *)"text/html" },  
	{(char *)"html",(char *)"text/html" },  
	{0,0} };

void logger(FILE *fp, int type, char *s1, char *s2, int socket_fd)
{
	
	FILE *fn=NULL;
	fn=fp;
	//char *logbuffer;
	char *nextline;
	//logbuffer=(char *)malloc(BUFSIZE*2);
	nextline=(char *)malloc(1);
	str_cpy(nextline,1,"\n");

	switch (type) {
	case ERROR: //asprintf(&logbuffer,"ERROR: %s:%s Errno=%d",s1, s2, errno); 
		(void)printf("error\n");
		fprintf(fn,"ERROR: %s:%s",s1, s2);
		break;
	case FORBIDDEN: 
		(void)printf("HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n");
		//(void)send(socket_fd,(void *)forbdn ,str_length(forbdn),0);
		//asprintf(&logbuffer,"FORBIDDEN: %s:%s",s1, s2); 
		fprintf(fn,"FORBIDDEN: %s:%s",s1, s2); 
		break;
	case NOTFOUND: 
		(void)printf("not found");
		//(void)send(socket_fd,(void *)notfound,224,0);
		//asprintf(&logbuffer,"NOT FOUND: %s:%s",s1, s2);
		fprintf(fn,"NOT FOUND: %s:%s",s1, s2);  
		break;
	case LOG://asprintf(&logbuffer," INFO: %s:%s:%d",s1, s2,socket_fd); 
		(void)printf("INSIDE LOGGER");
		fprintf(fn," INFO: %s:%s:%d",s1, s2,socket_fd); 
		//asprintf(&logbuffer,"INSIDE NWEB: %s:%s",s1, s2);

		break;
	}	
	/* No checks here, nothing can be done with a failure anyway */
	//if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		

		//snd=send(fd,(void *)logbuffer,str_length(logbuffer),0);
		 
		//(void)printf("%d r sent having %d fd\n",snd,fd);		
		
	
		//(void)send(fd,(void *)nextline,1,0);      
		
	
	if(type == ERROR || type == NOTFOUND || type == FORBIDDEN) exit(3);
}

/* this is a child web server process, so we can exit on errors */

static bool uri_is_valid(char *uri)
{
	if (uri[0] != '/')
		return false;
	
	if (uri[1] == '.')
		return false;
	
	char *cp = uri + 1;
	
	while (*cp != '\0') {
		char c = *cp++;
		if (c == '/')
			return false;
	}
	
	return true;
}
static int send_response(int conn_sd, const char *msg)
{
	size_t response_size = str_size(msg);
	
	if (verbose)
	    fprintf(stderr, "Sending response\n");
	
	ssize_t rc = send(conn_sd, (void *) msg, response_size, 0);
	if (rc < 0) {
		fprintf(stderr, "send() failed\n");
		return rc;
	}
	
	return EOK;
}
/** Receive one character (with buffering) */
static int recv_char(int fd, char *c)
{
	//(void)printf("in while recv_char\n");
	if (rbuf_out == rbuf_in) {
(void)printf("inside if\n");
		rbuf_out = 0;
		rbuf_in = 0;
		(void)printf("inside if\n");
		ssize_t rc = recv(fd, rbuf, BUFFER_SIZE, 0);
		(void)printf("inside if after recv function call\n");
		if (rc <= 0) {
			(void)printf("recv() failed\n");
			fprintf(stderr, "recv() failed (%zd)\n", rc);
			return rc;
		}
		(void)printf("recv success (%zd)\n",rc);
		
		rbuf_in = rc;
	}
	
	*c = rbuf[rbuf_out++];
	return EOK;
}
/** Receive one line with length limit */
static int recv_line(int fd)
{
	char *bp = lbuf;
	char c = '\0';
	
	while (bp < lbuf + BUFFER_SIZE) {
	//(void)printf("in while lbuf\n");
//(void)printf("BUFFER_SIZE %d\n",BUFFER_SIZE);
		char prev = c;
		int rc = recv_char(fd, &c);
		
		if (rc != EOK)
			return rc;
		
		*bp++ = c;
		if ((prev == '\r') && (c == '\n'))
			break;
	}
	
	lbuf_used = bp - lbuf;
	*bp = '\0';
	
	if (bp == lbuf + BUFFER_SIZE)
		return ELIMIT;
	
	return EOK;
}
static int uri_get(const char *uri, int conn_sd)
{
	if (str_cmp(uri, "/") == 0)
		uri = "/index.html";
	
	char *fname;
	int rc = asprintf(&fname, "%s%s", WEB_ROOT, uri);
	if (rc < 0)
		return ENOMEM;
	
	int fd = open(fname, O_RDONLY);
	if (fd < 0) {
		rc = send_response(conn_sd, msg_not_found);
		free(fname);
		return rc;
	}
	
	free(fname);
	
	rc = send_response(conn_sd, msg_ok);
	if (rc != EOK)
		return rc;
	
	while (true) {
		ssize_t nr = read(fd, fbuf, BUFFER_SIZE);
		if (nr == 0)
			break;
		
		if (nr < 0) {
			close(fd);
			return EIO;
		}
		
		rc = send(conn_sd, fbuf, nr, 0);
		if (rc < 0) {
			fprintf(stderr, "send() failed\n");
			close(fd);
			return rc;
		}
	}
	
	close(fd);
	
	return EOK;
}
static int req_process(int conn_sd)
{
	//(void)printf("in req_process()\n");	
	int rc = recv_line(conn_sd);
	//(void)printf("in recv_line()\n");
	if (rc != EOK) {
		fprintf(stderr, "recv_line() failed\n");
		return rc;
	}
	
	if (verbose)
		fprintf(stderr, "Request: %s", lbuf);
	
	if (str_lcmp(lbuf, "GET ", 4) != 0) {
			(void)printf("before send_response()\n");
		rc = send_response(conn_sd, msg_not_implemented);
			(void)printf("after send_response()\n");
		return rc;
	}
	
	char *uri = lbuf + 4;
	char *end_uri = str_chr(uri, ' ');
	if (end_uri == NULL) {
		end_uri = lbuf + lbuf_used - 2;
		assert(*end_uri == '\r');
	}
	
	*end_uri = '\0';
	if (verbose)
		fprintf(stderr, "Requested URI: %s\n", uri);
	
	if (!uri_is_valid(uri)) {
		rc = send_response(conn_sd, msg_bad_request);
		return rc;
	}
	
	return uri_get(uri, conn_sd);
}


int main(int argc, char **argv)
{	

	
	FILE *fp=NULL;
	fp=fopen("nweb.log","w+");
	if(fp!=NULL)
		(void)printf("open suces\n");
//printf("data in log bufr=%s\n",logbuffer);		
//while(logbuffer[i]!='\0')
//{		
//(void)fprintf(fp,"%c",logbuffer[i]);
//i++;
//}
//free(logbuffer);

	int i, port, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */

	if( argc < 3  || argc > 3 || !str_cmp(argv[1], "-?") ) {
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
	"\tnweb is a small and very safe mini web server\n"
	"\tnweb only servers out file/web pages with extensions named below\n"
	"\t and only from the named directory or its sub-directories.\n"
	"\tThere is no fancy features = safe and secure.\n\n"
	"\tExample: nweb 8181 /home/nwebdir &\n\n"
	"\tOnly Supports:", VERSION);
		for(i=0;extensions[i].ext != 0;i++)
			(void)printf(" %s",extensions[i].ext);

		(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
	"\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
	"\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"  );
		exit(0);
	}
	if( !str_lcmp(argv[2],"/"   ,2 ) || !str_lcmp(argv[2],"/etc", 5 ) ||
	    !str_lcmp(argv[2],"/bin",5 ) || !str_lcmp(argv[2],"/lib", 5 ) ||
	    !str_lcmp(argv[2],"/tmp",5 ) || !str_lcmp(argv[2],"/usr", 5 ) ||
	    !str_lcmp(argv[2],"/dev",5 ) || !str_lcmp(argv[2],"/sbin",6) ){
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n",argv[2]);
		exit(3);
	}
	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		exit(4);
	}
	//(void)printf("\nnweb starting\n");
	/* Become deamon + unstopable and no zombies children (= no wait()) */
	//if(fork() != 0)
		//return 0; /* parent returns OK to shell */
	//(void)posix_signal(SIGCLD, SIG_IGN); /* ignore child death */
	//(void)posix_signal(SIGHUP, SIG_IGN); /* ignore terminal hangups */
	// rupesh comment
	//for(i=0;i<32;i++)
	// this too	
	//(void)close(i);		/* close open files */
	//(void)setpgrp();		/* break away from process group */
	logger(fp, LOG,(char *)"nweb starting",argv[1],90);
	//(void)printf("\nnweb starting\n");
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0){
		logger(fp, ERROR, (char *)"system call",(char *)"socket",0);
		fprintf(stderr, "error Creating socket\n");	
		//(void)printf("error creating socket\n");	
	}
	//(void)printf("socket created\n");
	port = (int)8080;
	if(port < 0 || port >60000){
		logger(fp, ERROR,(char *)"Invalid port number (try 1->60000)",argv[1],0);
		(void)printf("invalid port number\n");
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){
		logger(fp,ERROR,(char *)"system call",(char *)"bind",0);
		(void)printf("error while binding\n");
	}
	(void)printf("binded successfully\n");
	if( listen(listenfd,64) <0){
		logger(fp, ERROR,(char *)"system call",(char *)"listen",0);
		(void)printf("error while listening to port\n");
	}
	fclose(fp);
	(void)printf("listening to port 8080\n");
	while(true){
		static struct sockaddr_in cli_addr; /* static = initialised to zeros */
		length = sizeof(cli_addr);
		int len2=sizeof(socketfd);
		(void)printf("socket size before %d\n",len2);		
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){
			logger(fp, ERROR,(char *)"system call",(char *)"accept",0);
			(void)printf("error while accepting from socket\n");
		}
		int len1=sizeof(socketfd);
		(void)printf("socket size %d\n",len1);
		(void)printf("accepted\n");
		rbuf_out = 0;
		rbuf_in = 0;
		int rc = req_process(socketfd);
		(void)printf("after req_process()\n");
		if (rc != EOK)
			(void)printf("Error processing request\n");
		(void)printf("rc %d\n", rc);
		rc = closesocket(socketfd);
		if (rc != EOK) {
			(void)printf("Error closing connection socket \n");
			closesocket(listenfd);
			return 5;
		}
		
		/*if((pid = fork()) < 0) {
			logger(ERROR,(char *)"system call",(char *)"fork",0);
		}
		else {
			if(pid == 0) { 	// child 
				(void)close(listenfd);*/
				//web(socketfd,hit); // never returns 
			/*} else { 	// parent 
				(void)close(socketfd);
			}
		}*/
	}
}
