//Racheli Chuwer
//211374111
#include "threadpool.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>

#define KILO 4000 //buffer size
#define MAX_LINE_SIZE 1024
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define VERSION "HTTP/1.0 "
#define USAGE "Usage: server <port> <pool-size> <max-number-of-request>\n"
void send_to_server(char* buffer,int f_socket,char* t_host, int port);
char **do_matrix(char* string);
int is_3_tokens(char * first_line);
void create_bad_response(int type , char * path, char* file_type,int * f_socket);
int handle_request(void* a);
int string_to_positive_number(char* str);
void send_response_to_client(int* f_socket, char* str);
int process_first_line(char* content, char c, int start);
void strip_extra_spaces(char* str);
char **hosts;
int num_hosts;
int length;

// //main
int main(int argc, char *argv[]){
char *filter=argv[4];
FILE * fp;
int i=0;
   char * line = NULL;
   size_t len = 0;
   int arr_len=0, j=0;
   ssize_t read;

   fp = fopen(filter, "r");
   if (fp == NULL)
   {
      perror("ERROR open file");
      exit(0);
   }
   while ((read = getline(&line, &len, fp)) != -1)
       arr_len++;
   hosts=malloc(arr_len);
   if(hosts==NULL)
   {
       fclose(fp);
       perror("ERROR malloc");
       exit(0);
   }
   line=NULL;
   rewind(fp);
   while ((read = getline(&line, &len, fp)) != -1)
   {
       hosts[j]=malloc(strlen(line)+1);
       if(hosts[j]==NULL)
       {
           fclose(fp);
           free(hosts);
           perror("ERROR malloc");
           exit(0);
       }
       strcpy(hosts[j], line);
       hosts[j][strlen(hosts[j])-1]='\0';
       j++;
       if (line)
           free(line);
   }
   num_hosts=arr_len;
   fclose(fp);
   for(i=0;i<num_hosts;i++)
   {
     hosts[i][strlen(hosts[i])-1]='\0';
   }
   if(argc != 5)
    {
        printf(USAGE);
        exit(1);
    }
	//validate args
    if(string_to_positive_number(argv[1])==-1 || string_to_positive_number(argv[2])==-1 || string_to_positive_number(argv[3])==-1) 
    {
        printf(USAGE);
        exit(1);
    }
    
    //create threadpool
    threadpool *t = create_threadpool(atoi(argv[2]));
    //create sockets for request
    int* newsocketfd = (int*)malloc(atoi(argv[3]) * sizeof(int));
    struct sockaddr_in server_address;
    struct sockaddr client_address;
    int client = sizeof(server_address);
    int socketfd;
	
	//open socket
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        destroy_threadpool(t);
        perror("socket\n");
		free(newsocketfd);
        exit(1);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));
    
    //binding 
    if (bind(socketfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        destroy_threadpool(t);
        perror("bind\n");
		free(newsocketfd);
        exit(1);
    }
	
	//listen for connections
    if(listen(socketfd, 5) < 0)
    {
        perror("listen\n");
        destroy_threadpool(t);
        close(socketfd);
		free(newsocketfd);
        exit(1);
    }

	//manage threadpools
	//int i;
    i=0;
    for(; i < atoi(argv[3]); i++)
    {
        if((newsocketfd[i] = accept(socketfd, &client_address, (socklen_t *)&client)) < 0)
        {
            destroy_threadpool(t);
            perror("accept\n");
            close(socketfd);
			free(newsocketfd);
            exit(1);
        }
        dispatch(t,handle_request,&newsocketfd[i]);
    }
	
    destroy_threadpool(t);
    close(socketfd);
	free(newsocketfd);
    return 0;
}

//handle request
int handle_request(void* request)
{
    int flag=0,time=0;
   char temp[4000];
   int i=0,port,num_letter=0;
   char* host, *tmp_host,buf2[100],*temp_port,*new_host,*t_host,**newString;
  // int * f_socket = (int*)request;
  int  f_socket = *((int*)request);
	int return_value = 0;
    char buffer[4000];
    char line[100];
    bzero(buffer,4000); //to avoid getting old data
	struct stat file_stat;
	int size, end1, end2, end3;
	char *text, *word1, *word2, *word3;

    do{
		return_value = read(f_socket, line, 100);
        strcat(buffer,line);
	}while((return_value!=0 && strstr(buffer,"\r\n\r\n")== NULL && strstr(buffer,"\n\n")== NULL));

    //empty request
    if(strcmp(buffer,"")==0){
        create_bad_response(400, NULL, "text/html", &f_socket);
        return 0;
    }


     ///process  text
    size = process_first_line(buffer, '\r',0);
    text = (char*)malloc((size+2) * sizeof(char));
    bzero(text,size+2); //to avoid getting old data
    strncpy(text,buffer,size+1);
    end1 = process_first_line(text, ' ', 0);
	word1 = (char*)malloc((end1+1) * sizeof(char));
    bzero(word1,end1+1); //to avoid getting old data
    strncpy(word1,text,end1);
    end2 = process_first_line(text,' ', end1+1);
    word2 = (char*)malloc((end2-end1+1) * sizeof(char));
    bzero(word2,end2-end1+1); //to avoid getting old data
    strncpy(word2,text+end1+2,end2-end1-2);
    end3 = process_first_line(text,'\r',end2+1);
    word3 = (char*)malloc((end3-end2+1) * sizeof(char));
    bzero(word3,end3-end2+1); //to avoid getting old data
    strncpy(word3,text+end2+1,end3-end2-1);
    //check if missing tokens
    if(is_3_tokens(text) == -1) {create_bad_response(400, word2, "text/html", &f_socket); flag=1;}
    //not HTTP/1.0 or HTTP/1.1
    else if((strcmp(word3,"HTTP/1.0")!=0) && (strcmp(word3,"HTTP/1.1")!=0)) {create_bad_response(400, word2, "text/html", &f_socket); flag=1; }
    //method is not GET
    else if(strcmp(word1,"GET")!=0) {create_bad_response(501, word2, "text/html", &f_socket); flag=1;}
    //empty path or path does not exists
    //else if(strcmp(word2,"")==0 || ((stat(word2, &file_stat) != 0) && (strlen(word2) != 0))) create_bad_response(404, word2, "text/html", f_socket);
    strcpy(temp, buffer);
    if((tmp_host=strstr(temp,"Host:"))==NULL) {create_bad_response(400, word2, "text/html", &f_socket); flag=1;}
     if(tmp_host!=NULL&&flag!=1){
         if(1==sscanf(tmp_host,"%*[^0123456789]%d",&port));
         else{port=80;}
         newString=do_matrix(temp);
        for(i=1;i<length;i++ )
        {  
        host = strtok(newString[i], ":");
        if(host == NULL)
            break;
        //printf("%s\n",token);
        /* walk through other tokens */
        if(strcmp(host,"Host")==0){
            break;
        }  
        }
        host = strtok(NULL, ":");
   
        t_host = malloc(sizeof(char)*(strlen(host)+1));
        strcpy(t_host,host);
        strip_extra_spaces(t_host);
        printf("666 %s 8888\n",t_host);
    }
    if(flag!=1){
    for(i=0;i<num_hosts;i++)
    {

    printf("*%s*\n",hosts[i]);
   /**/ //hosts[i][strlen(hosts[i])-1]='\0';
    printf("*%s*\n",hosts[i]);
    if(strcmp(t_host,hosts[i])==0)
     {
     flag=1;
     create_bad_response(403, word2, "text/html", &f_socket); 
     break;
     }
/**/
    }   
    }
    
    if(flag!=1){
     send_to_server(buffer,f_socket,t_host, port);
     
  
    }
    
 //--------------------------------------------------------------//
    free(text); free(word1); free(word2); free(word3);
    return 0;
}


//create response: 400, 404, 501 ,403
void create_bad_response(int type , char * path, char* file_type,int * f_socket){
    //create timestamp
    char timebuffer[128];
    bzero(timebuffer,128);
    time_t timestamp = time(NULL);
    strftime(timebuffer, sizeof(timebuffer), RFC1123FMT, gmtime(&timestamp));
	
	
	char error_code[256];
	char error_description[256];
	bzero(error_code,256);
    bzero(error_description,256);
	
	if(type==400){
		strcpy(error_code, "400 Bad Request");
		strcpy(error_description, "Bad Request.");
	}
	else if(type==404){
		strcpy(error_code, "404 Not Found");
		strcpy(error_description, "File not found.");
	}
	else if(type==501){
		strcpy(error_code, "501 Not supported");
		strcpy(error_description, "Method is not supported.");
	}
	else if(type==403){
		strcpy(error_code, "403 Forbidden");
		strcpy(error_description, "Access denied.");
	}
	
	//char* error_code = "this is an error code";
	//char* error_description = "this is an error description";
	
	char res[1000];
	strcpy(res, "<HTML><HEAD><TITLE>");
	strcat(res, error_code);
	strcat(res, "</TITLE></HEAD>\r\n<BODY><H4>");
	strcat(res, error_code);
	strcat(res, "</H4>\r\n");
	strcat(res, error_description);
	strcat(res, "\r\n</BODY></HTML>\r\n" );
	//create response content
	char text_size[1000];
	bzero(text_size, 1000); //to avoid getting old data
	sprintf(text_size,"%d\r\n" ,(int)strlen(res));
	int size_response = strlen(VERSION) + 
						strlen(error_code) +
						strlen("\r\nServer: webserver/1.0\r\n")+
						strlen("Date: ") + 
						strlen(timebuffer) + 
						strlen("\r\n") +
						strlen("Content-Type: ") +
						strlen(file_type) +
						strlen("\r\nContent-Length: ") +
						strlen(text_size) + 
						strlen("Connection: close\r\n\r\n")+
						strlen(res);

	char* content = (char*)malloc((size_response+1) * sizeof(char));
	bzero(content, size_response+1); //to avoid getting old data

	strcat(content, VERSION);
	strcat(content, error_code);
	strcat(content, "\r\nServer: webserver/1.0\r\n");
	strcat(content, "Date: ");
	strcat(content, timebuffer);
	strcat(content, "\r\n");
	strcat(content, "Content-Type: ");
	strcat(content, file_type);
	strcat(content, "\r\nContent-Length: ");
	strcat(content, text_size);
	strcat(content, "Connection: close\r\n\r\n");
	strcat(content, res);
	send_response_to_client(f_socket, content);
}
//send response to client
void send_response_to_client(int* f_socket, char* str){
    int return_value = 0, sum = 0;
	
    do{
        return_value = write(*f_socket, str, strlen((str)));
        sum += return_value;
        if (sum == strlen(str)){
			close(*f_socket);
			break;
		}
        if (return_value < 0){
            perror("write\n");
            close(*f_socket);
			free(str);
			return;
        }
    }while(1);
    close(*f_socket);
	free(str);
    exit(1);
}
//check if have in the first line 3 tokensss
int is_3_tokens(char * first_line){
    int count = 0;
    int l = strlen(first_line);
	int i;
    for(i = 0; i<l; i++)
        if(first_line[i] == ' ') count++;
    if(count==2) return 0;
    else return -1;
}
//check if a string is a positive number 
//return positive number or -1 if number not valid
int string_to_positive_number(char * str)
{
	int i;
    for (i = 0; i < strlen(str); i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return -1;
    }
    return atoi(str);
}
//process first line
int process_first_line(char* content, char c, int start){
    int end = strlen(content);
	int i;
    for(i = start ; i<end; i++)
        if(*(content+i) == c){return i;}
    return -1;
}


char **do_matrix(char* string)
{

char *temp = malloc(sizeof(char)*strlen(string)+1);
strcpy(temp,string);
int i = 0;
char ** new_str;
const char s[] = "\r\n";
char *token;
length = 0;
/* get the first token */
token = strtok(string, s);
/* walk through other tokens */
while (token != NULL)
{
    length++;
    token = strtok(NULL, s);
}
printf(" in the method %d\n",length);
new_str = (char**)malloc(sizeof(char*)*length);
if (new_str == NULL)
    printf("Error malloc\n");
/* get the first token */
token = strtok(temp, s);
//token[strlen(token) - 1] = '\0';
/* walk through other tokens */
while (token != NULL)
{
new_str[i] = token;
token = strtok(NULL, s);
//token[strlen(token) - 1] = '\0';
i++;
}
return new_str;
}


void strip_extra_spaces(char* str) {
  int i, x;
  for(i=x=0; str[i]; ++i)
   
if(str[i]!=' ' || (i > 0 && str[i-1]!=' '))
      str[x++] = str[i];
//if(i > 0 && str[i-1]=='\n')
// str[x] = str[i];
  str[x] = '\0';
}
void send_to_server(char *arg,int sock,char* host,int port)
{
   
    int sockfd,num,res,retur,k;
    struct hostent *server;
    char baffer[200];

    sockfd = socket(AF_INET,SOCK_STREAM,0);  //Create socket
    if(sockfd<0){   //Check that what was socket is correct
    
        perror("ERROR opening socket");
        exit(0);
    }
    server = gethostbyname(host);  //return the namber ip his
    if(server==NULL)
    {
        fprintf(stderr,"ERROR,no sach host\n");
        exit(0);
    }
    //definition the strackt
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(port);
    if(connect(sockfd,(const struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        perror("ERROR connecting");
        exit(0);
    }
    if(write(sockfd,arg,sizeof(char)*(strlen(arg)+1))<0){
       perror("ERROR writing");
       exit(0);
    }
   
    while(k = read(sockfd,&baffer,sizeof(char)*200)>0)    //read the answer from the server
        {
            retur = write(sock,baffer,200*sizeof(char));
            if(retur<0){
                perror("ERROR,ON WRITING");
                exit(0);
            }
        }
        if(k<0){
            perror("ERROR,ON READING");
            exit(0);
        }
        close(sockfd);
        close(sock);
}

