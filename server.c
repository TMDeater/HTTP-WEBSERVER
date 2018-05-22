#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <ctype.h>
#include "zfunc.c"

#define SERVER_PORT (12345)
#define LISTENNQ (5)
#define MAXLINE (100)
#define MAXTHREAD (5)
#define THREADSTACK 65536

struct Conn {
  int* listenfd;
  int* connfd;
};

void* request_func(void* con);

int finddir(char *dir, int depth, char* filename)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    if((dp = opendir(dir)) == NULL) {
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }
    chdir(dir);
    printf("find in directory\n");
    printf("requesting filename: %s\n",filename);
    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        //printf("Compare %s with %s\n",entry->d_name,filename);
        if (strcmp(entry->d_name,filename)==0){
          printf("***found file\n");
          return 1;
        }
        if(strcmp(".",entry->d_name) == 0 ||
            strcmp("..",entry->d_name) == 0)
            continue;
    }
    //chdir("..");
    closedir(dp);
    return -1;
}

int compressfile(char* filename,char* outputname){
  int ret;
  FILE *source;
  FILE *zip;
  FILE *zipped;
  FILE *back;

  source = fopen (filename, "r");
  if (source==NULL){
    printf("Read File doesn't exist\n");
    return -1;
  }
  zip = fopen (outputname, "wx");
  if(zip==NULL){
    printf("zip already exist\n");
    return 1;
  }

  printf ("calling def \n");
  ret = def(source, zip, Z_DEFAULT_COMPRESSION);
  printf("def return: %i \n", ret);
  fclose(source);
  fclose(zip);

   fclose(source);
   fclose(zip);
   return 1;
}

int main(int argc, char **argv)
{
        int on = 1;
        int listenfd, connfd;

        struct sockaddr_in servaddr, cliaddr;
        socklen_t len = sizeof(cliaddr);

        char ip_str[INET_ADDRSTRLEN] = {0};

      	int threads_count = 0;
      	//pthread_t* threads = malloc(sizeof(pthread_t)*MAXTHREAD);
        pthread_t threads[MAXTHREAD];
        pthread_attr_t attrs;
        pthread_attr_init(&attrs);
        pthread_attr_setstacksize(&attrs,THREADSTACK);
        //printf("Server start\n");

        /* initialize server socket */
        listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
        if (listenfd < 0) {
                printf("Error: init socket\n");
                return 0;
        }

        setsockopt(listenfd,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
        /* initialize server address (IP:port) */
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
        servaddr.sin_port = htons(SERVER_PORT); /* port number */
        printf("set socket\n");

        /* bind the socket to the server address */
        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) < 0) {
                printf("Error: bind\n");
                return 0;
        }

        if (listen(listenfd, LISTENNQ) < 0) {
                printf("Error: listen\n");
                return 0;
        }

        /* keep processing incoming requests */
        while (1){
          if(!fork()){
            //child
            while (connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len)) {

                  /* accept an incoming connection from the remote side */

                  printf("Accept client, connfd:%d\n", connfd);
                  if (connfd < 0) {
                          printf("Error: accept\n");
                          return 0;
                  }

                  /* print client (remote side) address (IP : port) */
                  inet_ntop(AF_INET, &(cliaddr.sin_addr), ip_str, INET_ADDRSTRLEN);
                  printf("Incoming connection from %s : %hu with fd: %d\n", ip_str, ntohs(cliaddr.sin_port), connfd);

                  // if (!fork()){
                    /* create dedicate thread to process the request */
                  // struct Conn* con;
                  // con->listenfd = malloc(1);
                  // *con->listenfd = listenfd;
                  // con->connfd = malloc(1);
                  // *con->connfd = connfd;

                    // request_func(con);
                    // exit(0);
                  // printf("before thread created\n");
              		if (pthread_create(&threads[threads_count], &attrs, request_func, (void*) connfd)!=0) {
              			printf("Error when creating thread %d\n", threads_count);
              			return 1;
              		}
                  //pthread_join(threads[0],NULL);
                  //threads_count++;
                  printf("Created thread\n");
              		if (++threads_count >= MAXTHREAD) {
              			break;
              		}
            }
            int i=0;
            for (i=0;i<MAXTHREAD;i++){
              printf("Thread exceed 5, wait for %d finish...", i);
              pthread_join(threads[i],NULL);
              printf("OK!\n");
            }
            threads_count=0;
            printf("child exit\n");
            exit(0);
          } else {
            //parent
            wait(0);
          }

        }


        return 0;
}

void* request_func(void* con)
{
  int connfd = (int)con;
  // struct Conn conn = *(struct Conn*)con;
  // int listenfd = *conn.listenfd;
  // int connfd = *conn.connfd;
  // printf("In function listenfd: %d\n",listenfd);
  printf("In function connfd: %d\n", connfd);
  // close(listenfd);

	/* get the connection fd */
  int fdimg;
  struct stat stat_len;
  char readRequest[2048];
  int recvMsgLen;
  memset(readRequest,0,2047);
  printf("In function mem set\n");
  //if(!read(connfd, readRequest, 2047)){printf("Cannot read");}
  if ((recvMsgLen = recv(connfd, readRequest, 2048, 0))<0){
    strcpy(readRequest,"none");
  }
  //printf("In function read\n");
  printf("ReadRequest is\n\n%s\n\n", readRequest);
  //
  //find request filename
  int j=5;
  int copyStrCount = 0;
  char requestfilename[20];
  while(readRequest[j]!=' '){
    requestfilename[copyStrCount]=readRequest[j];
    copyStrCount++;
    j++;
  }
  requestfilename[copyStrCount]='\0';

  //find file type
  char filetype[10];
  j = strlen(requestfilename)-1;
  while (requestfilename[j]!='.' && j>0){
    j--;
  }
  j++;
  int strcount = 0;
  while (requestfilename[j]!='\0'){
    filetype[strcount]=tolower(requestfilename[j]);
    j++;
    strcount++;
  }
  filetype[strcount]='\0';

  if(!strncmp(readRequest, "GET /favicon.ico", 16)){
    fdimg = open("favicon.ico", O_RDONLY);
    fstat(fdimg,&stat_len);
    write(connfd, "HTTP/1.1 200 OK\r\n", 17);
    write(connfd, "Content-Type: image/x-icon\n\n", 28);
    sendfile(connfd, fdimg, 0, 5848);
    close(fdimg);
  // } else if(!strncmp(readRequest, "GET /future.PNG", 15)) {
  //   // fdimg = open("future.PNG", O_RDONLY);
  //   // fstat(fdimg,&stat_len);
  //   // write(connfd, "HTTP/1.1 200 OK\r\n", 17);
  //   // write(connfd, "Content-Type: image/png\n\n", 25);
  //   // sendfile(connfd, fdimg, 0, 157326);
  //   // close(fdimg);
  //   compressfile("future.PNG","future.gz");
  //   fdimg = open("future.gz", O_RDONLY);
  //   write(connfd, "HTTP/1.1 200 OK\r\n", 17);
  //   write(connfd, "Content-Type: image/png\r\n", 25);
  //   write(connfd, "Content-Encoding: gzip\n\n", 24);
  //   sendfile(connfd, fdimg, 0, 157326);
  //   close(fdimg);
  }else if(!strncmp(readRequest, "GET /1.pdf", 10)){
    int pdffile = open("1.pdf", O_RDONLY);
    fstat(pdffile,&stat_len);
    write(connfd, "HTTP/1.1 200 OK\r\n", 17);
    write(connfd, "Content-Type:application/pdf\n\n", 30);
    sendfile(connfd, pdffile, 0, 1469096);
    close(pdffile);
  }else if (!strncmp(readRequest, "GET /index.css", 14)){
    write(connfd, "HTTP/1.1 200 OK\n", 16);
    write(connfd, "Transfer-Encoding: chunked\r\n", 28);
    write(connfd, "Content-Type: text/css\r\n", 25);
    write(connfd, "\r\n",2);
    write(connfd, "\r\n",2);
    FILE *fp;
    char* line = NULL;
    size_t line_len = 0;
    ssize_t rd;

    fp = fopen("index.css","r");
    if (fp==NULL){perror("Error open index.css\n");exit(EXIT_FAILURE);}
    while((rd=getline(&line,&line_len,fp))!=-1){
      char tmp[50];
      int hexLen = sprintf(tmp,"%x", rd);
      write(connfd,tmp, hexLen);
      write(connfd,"\r\n",2);
      write(connfd,line, rd);
      write(connfd,"\r\n",2);
      //printf("%s\n",tmp);
      //printf("%s\n",line);
    }
    write(connfd,"0\r\n",3);
    write(connfd, "\r\n",2);
    fclose(fp);
  }else if(!strncmp(readRequest, "GET / HTTP/1.1", 14)){
    printf("heavy computation\n");

    write(connfd, "HTTP/1.1 200 OK\n", 16);
    write(connfd, "Transfer-Encoding: chunked\r\n", 28);
    write(connfd, "Content-Type: text/html;\r\n", 26);
    write(connfd, "\r\n", 2);

    /* prepare for the send buffer */
    FILE *fp;
    char* line = NULL;
    size_t line_len = 0;
    ssize_t rd;

    fp = fopen("index.html","r");
    if (fp==NULL){perror("Error open index.html\n");exit(EXIT_FAILURE);}
    while((rd=getline(&line,&line_len,fp))!=-1){
      char tmp[50];
      int hexLen = sprintf(tmp,"%x", rd);
      write(connfd,tmp, hexLen);
      write(connfd,"\r\n",2);
      write(connfd,line, rd);
      write(connfd,"\r\n",2);
    }
    write(connfd,"0\r\n",3);
    write(connfd, "\r\n",2);
    fclose(fp);

    printf("Printed\n");
  }
  else if(strcmp(filetype, "jpg")==0 ||
          strcmp(filetype, "gif")==0 ||
          strcmp(filetype, "png")==0){
    int findImg = finddir("/home/oslab/COMP4621SER",0,requestfilename);
    if (findImg==1){

      if (strstr(readRequest,"Accept-Encoding: gzip")!=NULL){
        char newFilename[50];
        sprintf(newFilename,"%s.gz",requestfilename);
        printf("****Compress %s\n",newFilename);
        compressfile(requestfilename,newFilename);
        fdimg = open(newFilename, O_RDONLY);
        FILE* f = fopen(requestfilename,"r");
        fseek(f, 0, SEEK_END);
        int siz = (int) ftell(f);
        printf("file size is %d", siz);
        char tmpstr[35];
        sprintf(tmpstr, "Content-Type: image/%s\r\n", filetype);
        write(connfd, "HTTP/1.1 200 OK\r\n", 17);
        write(connfd, tmpstr, strlen(tmpstr));
        write(connfd, "Content-Encoding: gzip\n\n", 24);
        sendfile(connfd, fdimg, 0, siz);
        close(fdimg);
      } else {
        fdimg = open(requestfilename, O_RDONLY);
        FILE* f = fopen(requestfilename,"r");
        fseek(f, 0, SEEK_END);
        int siz = (int) ftell(f);
        printf("file size is %d", siz);
        char tmpstr[35];
        sprintf(tmpstr, "Content-Type: image/%s\n\n", filetype);
        write(connfd, "HTTP/1.1 200 OK\r\n", 17);
        write(connfd, tmpstr, strlen(tmpstr));
        sendfile(connfd, fdimg, 0, siz);
        close(fdimg);
      }
    }
  }
  else if (finddir("/home/oslab/COMP4621SER",0,requestfilename)==-1){
    printf("404 not found\n");
    write(connfd, "HTTP/1.1 404 NOT FOUND\r\n", 24);
    write(connfd, "Transfer-Encoding: chunked\r\n", 28);
    write(connfd, "Content-Type: text/html;\r\n", 26);
    write(connfd, "\r\n",2);
    write(connfd, "33\n",3);
    write(connfd, "<html><body><H1>The file or site is not found</H1>\r\n",52);
    write(connfd, "20\r\n",4);
    write(connfd, "<html><body><img src=\"404.jpg\">\r\n",33);
    write(connfd, "f\r\n",3);
    write(connfd, "</body></html>\r\n", 16);
    write(connfd, "0\r\n",3);
    write(connfd, "\r\n",2);
  }
  else if(finddir("/home/oslab/COMP4621SER",0,requestfilename)==1){
    write(connfd, "HTTP/1.1 200 OK\n", 16);
    write(connfd, "Transfer-Encoding: chunked\r\n", 28);
    write(connfd, "Content-Type: text/html;\r\n", 26);
    write(connfd, "\r\n", 2);
    FILE *fp;
    char* line = NULL;
    size_t line_len = 0;
    ssize_t rd;

    fp = fopen(requestfilename,"r");
    if (fp==NULL){perror("Error open index.html\n");exit(EXIT_FAILURE);}
    while((rd=getline(&line,&line_len,fp))!=-1){
      char tmp[50];
      int hexLen = sprintf(tmp,"%x", rd);
      write(connfd,tmp, hexLen);
      write(connfd,"\r\n",2);
      write(connfd,line, rd);
      write(connfd,"\r\n",2);
    }
    write(connfd,"0\r\n",3);
    write(connfd, "\r\n",2);
    fclose(fp);
  }else if (readRequest==NULL){
    printf("cannot read\n");
  } else {

  }

  //free(conn.listenfd);
  //free(conn.connfd);

	close(connfd);
  pthread_exit((void*)0);
  return 0;
}
