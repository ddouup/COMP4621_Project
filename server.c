#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <time.h>
#include <assert.h>

#define MAXLINE 1000
#define SERVER_PORT 12345
#define LISTENNQ 5
#define MAXTHREAD 10
#define CHUNKSIZE 128

int threads_count = 0;

void* request_func(void *args);
char* file_ext(const char *string);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct thread_info *tinfo;

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    char ip_str[INET_ADDRSTRLEN];
    char wrt_buff[MAXLINE], rcv_buff[MAXLINE];

    pthread_t threads[MAXTHREAD];

    /* initialize server socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0); /* SOCK_STREAM : TCP */
    if (listenfd < 0) {
            printf("Error: init socket\n");
            return 0;
    }

    /* initialize server address (IP:port) */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; /* IP address: 0.0.0.0 */
    serv_addr.sin_port = htons(SERVER_PORT); /* port number */

    /* bind the socket to the server address */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
            printf("Error: bind\n");
            return 0;
    }

    if (listen(listenfd, LISTENNQ) < 0) {
            printf("Error: listen\n");
            return 0;
    }

    /* keep processing incoming requests */
    while (1) {
            /* accept an incoming connection from the remote side */
            connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &len);
            if (connfd < 0) {
                    printf("Error: accept\n");
                    return 0;
            }
            memset(&ip_str, 0, sizeof(ip_str));
            //memset(&wrt_buff, 0, sizeof(wrt_buff));
            //memset(&rcv_buff, 0, sizeof(rcv_buff));

            /* print client (remote side) address (IP : port) */
            inet_ntop(AF_INET, &(cli_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
            printf("Incoming connection from %s : %hu\n", ip_str, ntohs(cli_addr.sin_port));

            //struct thread_info args;

            if (pthread_create(&threads[threads_count], NULL, request_func, (void *)connfd) != 0) {
                printf("Error when creating thread %d\n", threads_count);
                return 0;
            }

            if (++threads_count >= MAXTHREAD) {
                break;
            }
            
    }
    printf("Max thread number reached, wait for all threads to finish and exit...\n");
    int i;
    for (i = 0; i < MAXTHREAD; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

char* file_ext(const char *string)
{
    assert(string != NULL);
    char *ext = strrchr(string, '.');
 
    if (ext == NULL)
        return (char*) string + strlen(string);
 
    for (char *iter = ext + 1; *iter != '\0'; iter++) {
        if (!isalnum((unsigned char)*iter))
            return (char*) string + strlen(string);
    }
 
    return ext;
}

void* request_func(void *args)
{
    //struct thread_info *tinfo = args;
    int connfd, bytes_rcv, bytes_wrt, total_bytes_wrt, fsize, count;
    char wrt_buff[MAXLINE], rcv_buff[MAXLINE];
    char chunk_buff[CHUNKSIZE];

    FILE *file;
    char filename[128];
    char method[16];
    char *ext, *type;
    int index = 0;

    int CHUNK = 1;
    int GZIP = 0;

    /* get the thread info */
    connfd = (int)args;
    memset(&wrt_buff, 0, sizeof(wrt_buff));
    memset(&rcv_buff, 0, sizeof(rcv_buff));

    /* read the response */
    bytes_rcv = 0;
    while (1) {
            bytes_rcv += recv(connfd, rcv_buff + bytes_rcv, sizeof(rcv_buff) - bytes_rcv - 1, 0);
            /* terminate until read '\n' */
            if (bytes_rcv && rcv_buff[bytes_rcv - 1] == '\n')
                    break;
    }

    /* parse request */
    sscanf(rcv_buff, "%s %s\n", method, filename);

    if(strcmp(filename, "/")==0){
        strcpy(filename, "/index.html.gz");
    }
    strcpy(filename,filename+1);
    ext = file_ext(filename);
    printf("Extension %s\n", ext);
    if (strcmp(ext, ".html")==0)
        type = "text/html";
    else if (strcmp(ext, ".css")==0)
        type = "text/css";
    else if (strcmp(ext, ".jpg")==0)
        type = "image/jpg";
    else if (strcmp(ext, ".pdf")==0)
        type = "application/pdf";
    else if (strcmp(ext, ".pptx")==0)
        type = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    else if (strcmp(ext, ".gz")==0){
        GZIP = 1;
        type = "";
    }
    else{
        type = "";
        printf("Invaild type" );
    }
    printf("Type %s\n", type);
    printf("Reading from file '%s' ...\n", filename);
    
    file = fopen(filename, "r");
    if (!file) {
        printf("404 Not Found\n\n");
        snprintf(wrt_buff, sizeof(wrt_buff) - 1, 
        "HTTP/1.1 404 Not Found\r\n\
        Server: COMP4621_Project\r\n\
        Content-Type: text/html\r\n\r\n\
        <!DOCTYPE html>\
        <html>\
        <head>\
        <title>404 Not Found</title>\
        </head>\
        <body><div>\
        <h1 style='font-weight: bold'>404 Not Found</h1>\
        <p>The requested URL was not found on this server</p>\
        </div></body>\
        </html>");
        write(connfd, wrt_buff, strlen(wrt_buff));
    } else {
        printf("200 OK\n\n");
        //get file size
        fseek(file, 0L, SEEK_END);
        fsize = ftell(file);
        rewind(file);

        snprintf(wrt_buff, sizeof(wrt_buff) - 1, "HTTP/1.1 200 OK\r\n");
        write(connfd, wrt_buff, strlen(wrt_buff));

        snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Server: COMP4621_Project\r\n");
        write(connfd, wrt_buff, strlen(wrt_buff));

        if (CHUNK)
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Transfer-Encoding: chunked\r\n");
        else
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Content-Length: %d\r\n", fsize);
        write(connfd, wrt_buff, strlen(wrt_buff));
        
        if(type != ""){
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Content-Type: %s\r\n", type);
            write(connfd, wrt_buff, strlen(wrt_buff));
        }

        if (GZIP){
            snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Content-Encoding: gzip\r\n");
            write(connfd, wrt_buff, strlen(wrt_buff));
        }

        snprintf(wrt_buff, sizeof(wrt_buff) - 1, "Keep-Alive: timeout=5, max=100\r\nConnection: Keep-Alive\r\n\r\n");
        write(connfd, wrt_buff, strlen(wrt_buff));

        if (CHUNK) {
            count = fread(chunk_buff, 1, CHUNKSIZE, file);
            while (count == CHUNKSIZE) {
                snprintf(wrt_buff, sizeof(wrt_buff) - 1, "%04X\r\n", count);
                write(connfd, wrt_buff, strlen(wrt_buff));
                write(connfd, chunk_buff, count);
                write(connfd, "\r\n", 2);
                count = fread(chunk_buff, 1, CHUNKSIZE, file);
            }
            if (count != 0) {
                snprintf(wrt_buff, sizeof(wrt_buff) - 1, "%04X\r\n", count);
                write(connfd, wrt_buff, strlen(wrt_buff));
                write(connfd, chunk_buff, count);
                write(connfd, "\r\n", 2);
            }
            write(connfd, "0\r\n\r\n", 5);
        }
        else {
            index=0;
            while ((wrt_buff[index] = fgetc(file)) != EOF) {
                ++index;
                }

            bytes_wrt = 0;
            total_bytes_wrt = strlen(wrt_buff);
            while (bytes_wrt < total_bytes_wrt) {
                bytes_wrt += write(connfd, wrt_buff + bytes_wrt, total_bytes_wrt - bytes_wrt);
            }
        }
        fclose(file);
    }

    /* close the connection */
    close(connfd);
    threads_count--;
}
