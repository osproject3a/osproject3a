#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <str.h>
#include <fcntl.h>
#include "../../posix/include/posix/signal.h"
#include <sys/types.h>
#include <sys/time.h>
#include "../../lib/c/include/net/socket.h"
#include "../../lib/c/include/net/in.h"
#include "../../lib/c/include/net/inet.h"
#include <str_error.h>
#include <assert.h>

#define VERSION 23

#define ERROR      42
#define LOG        44
#define MSG_NOT_IMPLEMENTED 501
#define NOTFOUND  404
#define MSG_BAD_REQUEST 400

#define DEFAULT_PORT  8080
#define BACKLOG_SIZE  3

#define WEB_ROOT  "/data/web"

/** Buffer for receiving the request. */
#define BUFFER_SIZE  8096
FILE *fp=NULL;
//FILE *fs=NULL;

struct timeval tv; 


void logger(FILE *fp,int type, char *s1, char *s2, int socket_fd);

static char rbuf[BUFFER_SIZE];

static size_t out_buffer;
static size_t in_buffer;

static char lbuf[BUFFER_SIZE + 1];
static size_t lbuf_used;

static char fbuf[BUFFER_SIZE];

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
    "<p>Please try again.</p>\r\n"
    "<p>Please check the name of the file given in the request</p>\r\n"
    "<p>This message is delivered by the web server</p>\r\n"
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
	{(char *)"mp3",(char *)"audio/mpeg" },   
	{0,0} };

void logger(FILE *fp, int type, char *s1, char *s2, int socket_fd)
{
	
	FILE *fn=NULL;
	fn=fp;
	char buffer[30];
	time_t curtime;
	
char *nextline;
	nextline=(char *)malloc(1);
	str_cpy(nextline,1,"\n");

	switch (type) {
	case ERROR: 
		(void)printf("error\n");
		gettimeofday(&tv, NULL);		
		curtime=tv.tv_sec;		
		time_local2str(curtime,buffer);
		(void)printf("%s%ld\n",buffer,tv.tv_sec);
		fprintf(fn,"ERROR: %s:%s",s1, s2);
		fprintf(fn,"timestamp full:%s \n",buffer);	
			
		break;
	case MSG_BAD_REQUEST: 
		(void)printf("message bad request\n");
		gettimeofday(&tv, NULL);		
		curtime=tv.tv_sec;		
		time_local2str(curtime,buffer);
		(void)printf("%s%ld\n",buffer,tv.tv_sec);
		fprintf(fn,"MSG_BAD_REQUEST: %s:%s",s1, s2);
		fprintf(fn,"timestamp full:%s \n",buffer);	
		
		break;
	case MSG_NOT_IMPLEMENTED: 
		(void)printf("HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n");
		gettimeofday(&tv, NULL);		
		curtime=tv.tv_sec;		
		time_local2str(curtime,buffer);
		(void)printf("%s%ld\n",buffer,tv.tv_sec);
		fprintf(fn,"MSG_NOT_IMPLEMENTED: %s:%s",s1, s2); 
		fprintf(fn,"timestamp full:%s \n",buffer);	
		
		break;
	case NOTFOUND: 
		(void)printf("not found");
		gettimeofday(&tv, NULL);		
		curtime=tv.tv_sec;		
		time_local2str(curtime,buffer);
		(void)printf("%s%ld\n",buffer,tv.tv_sec);
		fprintf(fn,"NOT FOUND: %s:%s",s1, s2); 
		fprintf(fn,"timestamp full:%s \n",buffer);	
		 
		break;
	case LOG:
		(void)printf("INSIDE LOGGER");
		gettimeofday(&tv, NULL);		
		curtime=tv.tv_sec;		
		time_local2str(curtime,buffer);
		(void)printf("%s%ld\n",buffer,tv.tv_sec);
		fprintf(fn," INFO: %s:%s:%d \n",s1, s2,socket_fd);
		fprintf(fn,"timestamp full:%s \n",buffer);	
		 
		break;
	}	
		
	if(type == ERROR || type == MSG_NOT_IMPLEMENTED) exit(3);
}

/* this is a child web server process, so we can exit on errors */

static bool check(char *url)
{
	if (url[0] != '/')
		return false;
	
	
	
	char *check = url + 1;
	
	while (((int)*check) != 0) {
		//printf("%c",*check);
		char c = *check;
		
	
		if ((int)c <97 || (int)c >122){
			if((int)c == 46||((int)c==51)){}
			else
			return false;
		}
		check++;
	}
	
	return true;
}
static int send_response(int conn_sd, const char *msg)
{
	size_t response_size = str_size(msg);
	
		
	ssize_t rc = send(conn_sd, (void *) msg, response_size, 0);
	if (rc < 0) {
		fprintf(stderr, "send() failed\n");
		return rc;
	}
	
	return EOK;
}
/** Receive one character (with buffering) */

static int extract_char(int fd, char *c)
{	
	//printf("out_buffer %u",out_buffer);
	//printf("in_buffer %u",in_buffer);
	if (out_buffer == in_buffer) {

		out_buffer = 0;
		in_buffer = 0;
		(void)printf("inside if\n");
		ssize_t rc = recv(fd, rbuf, BUFFER_SIZE, 0);
		if (rc <= 0) {
			(void)printf("recv() failed\n");
			fprintf(stderr, "recv() failed (%zd)\n", rc);
			return rc;
		}
		
		(void)printf("recv success (%zd)\n",rc);
		
		in_buffer = rc;
		
	}
	
	*c = rbuf[out_buffer++];
	//printf("c %c",*c);
	return EOK;
}

/** Receive one line with length limit */
static int extract_line(int fd)
{
	
	char *bp = lbuf;
	char c = '\0';
	
	while (bp < lbuf + BUFFER_SIZE) {
		char prev = c;
		int rc = extract_char(fd, &c);
		
		if (rc != EOK)
			return rc;
		
		*bp++ = c;
		if ((prev == '\r') && (c == '\n'))
			break;
	}
	//printf("lbuf is : %s",lbuf);
	lbuf_used = bp - lbuf;
	*bp = '\0';
	
	if (bp == lbuf + BUFFER_SIZE)
		return ELIMIT;
	
	return EOK;
}
static int get_url(const char *url, int conn_sd)
{
	
	
	char *fname;
	int rc = asprintf(&fname, "%s%s", WEB_ROOT, url);
	if (rc < 0)
		return ENOMEM;
	
	int fd = open(fname, O_RDONLY);
	if (fd < 0) {
		rc = send_response(conn_sd, msg_not_found);
		free(fname);
		fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
		logger(fp, NOTFOUND,(char *)"message",(char *)"not found",90);
		fclose(fp);
		
		return rc;
	}
	
	free(fname);
	
	rc = send_response(conn_sd, msg_ok);
	fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
	logger(fp, LOG,(char *)"message",(char *)"ok",90);
	fclose(fp);
		
	
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
static int web(int conn_sd)
{	
	
	int rc = extract_line(conn_sd);
	if (rc != EOK) {
		fprintf(stderr, "extract_line() failed\n");
		return rc;
	}
	
	
	
	if (str_lcmp(lbuf, "GET ", 4) != 0) {
			(void)printf("before send_response()\n");
		rc = send_response(conn_sd, msg_not_implemented);
		fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
		logger(fp, MSG_NOT_IMPLEMENTED,(char *)"only simple get",(char *)"supported",90);
		fclose(fp);		
		
			(void)printf("after send_response()\n");
		return rc;
	}
	
	char *url = lbuf + 4;
	//printf("url is :%s",url);
	char *end_url = str_chr(url, ' ');
	if (end_url == NULL) {
		end_url = lbuf + lbuf_used - 2;
		assert(*end_url == '\r');
	}
	
	*end_url = '\0';
	
	
	if (!check(url)) {
		rc = send_response(conn_sd, msg_bad_request);
		fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
		logger(fp, MSG_BAD_REQUEST,(char *)"url should not contain",(char *)"special characters",90);
	fclose(fp);
		return rc;
	}
	
	return get_url(url, conn_sd);
}


int main(int argc, char **argv)
{	

	
	
	//opening log file for log entries
	fp=fopen("nweb.log","a");
	if(fp!=NULL)
		(void)printf("open success\n");

	int i, port, listenfd, socketfd;
	socklen_t length;
	static struct sockaddr_in serv_addr; /* static = initialised to zeros */
	
	/*
	 * If command p argument passed is not valid, error is displayed and programme exits
	 * */
	if( argc < 3  || argc > 3 || !str_cmp(argv[1], "-?") ) {
		(void)printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n"
	"\tnweb is a small and very safe mini web server\n"
	"\tnweb only servers out file/web pages with extensions named below\n"
	"\t and only from the named directory or its sub-directories.\n"
	"\tThere is no fancy features = safe and secure.\n\n"
	"\tExample: nweb 8181 /home/nwebdir &\n\n"
	"\tOnly Supports:", VERSION);
	
	/*
	 * Checks if extension is supported in nweb.If not supported error is displayed and programme exits
	 * */
		for(i=0;extensions[i].ext != 0;i++)
			(void)printf(" %s",extensions[i].ext);

		(void)printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
	"\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin \n"
	"\tNo warranty given or implied\n\tNigel Griffiths nag@uk.ibm.com\n"  );
		exit(0);
	}
	/*
	 * Current directory check. If not current directory, programme exits.
	 * */
	if( !str_lcmp(argv[2],"/"   ,2 ) || !str_lcmp(argv[2],"/etc", 5 ) ||
	    !str_lcmp(argv[2],"/bin",5 ) || !str_lcmp(argv[2],"/lib", 5 ) ||
	    !str_lcmp(argv[2],"/tmp",5 ) || !str_lcmp(argv[2],"/usr", 5 ) ||
	    !str_lcmp(argv[2],"/dev",5 ) || !str_lcmp(argv[2],"/sbin",6) ){
		(void)printf("ERROR: Bad top directory %s, see nweb -?\n",argv[2]);
		exit(3);
	}
	/*
	 * to change to the current directory.
	 * */
	if(chdir(argv[2]) == -1){ 
		(void)printf("ERROR: Can't Change to directory %s\n",argv[2]);
		exit(4);
	}
	
	/*
	 * Function call for storing the logger messages
	 * */
	
	logger(fp, LOG,(char *)"nweb starting",argv[1],90);
	
	/* setup the network socket */
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0){
		logger(fp, ERROR, (char *)"system call",(char *)"socket",0);
		fprintf(stderr, "error Creating socket\n");	
		
	}
	
	/*
	 * Checks for the validity of the port
	 * */
	port = (int)8080;
	if(port < 0 || port >60000){
		logger(fp, ERROR,(char *)"Invalid port number (try 1->60000)",argv[1],0);
		(void)printf("invalid port number\n");
	}
	/*
	 * Selects TCP-IP Protocol and takes  ip address of the host system.
	 * */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*
	 * convert values between host and network byte order
	 * */
	serv_addr.sin_port = htons(port);
	/*
	 * binds associates a local address with a socket.
	 * */
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){
		logger(fp,ERROR,(char *)"system call",(char *)"bind",0);
		(void)printf("error while binding\n");
	}
	(void)printf("binded successfully\n");
	
	/*
	 * If no socket connection is made, error is displayed
	 * */
	if( listen(listenfd,64) <0){
		logger(fp, ERROR,(char *)"system call",(char *)"listen",0);
		(void)printf("error while listening to port\n");
	}
	fclose(fp);
	(void)printf("listening to port 8080\n");
	
	/*
	 * Loop for responding to multiple requests
	 * */
	while(true){
		static struct sockaddr_in cli_addr; /* static = initialised to zeros */
		length = sizeof(cli_addr);
		int len2=sizeof(socketfd);
		(void)printf("socket size before %d\n",len2);
		fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
		logger(fp, LOG,(char *)"logging ",(char *)"client socket created\n",90);
		fclose(fp);
		/*
		 * listens to the request on created socket from client and logs the error details if any*/	
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){
			
			fp = fopen("nweb.log","a");
			if(fp!=NULL)
			(void)printf("open success\n");	
			logger(fp, ERROR,(char *)"system call",(char *)"accept",0);
			fclose(fp);
			(void)printf("error while accepting from socket\n");
			
		}
		
		int len1=sizeof(socketfd);
		(void)printf("socket size %d\n",len1);
		(void)printf("accepted\n");
		out_buffer = 0;
		in_buffer = 0;
		/*receive request ,reads it line by line and sends the requested page to client
		 * */
		int rc = web(socketfd);
		(void)printf("after web()\n");
		/*If request is processed succesfully EOK is returned, else error*/
		if (rc != EOK)
			(void)printf("Error processing request\n");
		(void)printf("rc %d\n", rc);
		/*closing the socket*/
		rc = closesocket(socketfd);
		if (rc != EOK) {
			(void)printf("Error closing connection socket \n");
			closesocket(listenfd);
			return 5;
		}
		
	}
}
