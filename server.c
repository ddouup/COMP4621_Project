#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>

#define MAXLINE 100
#define SERVER_PORT 12345
#define LISTENNQ 5

struct thread_info {    /* Used as argument to request_func() */
    int connfd;
    char wrt_buff[MAXLINE];
    char rcv_buff[MAXLINE];
};

int main(int argc, char **argv)
{
        int listenfd, connfd;
        struct thread_info *tinfo;

        struct sockaddr_in serv_addr, cli_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        char ip_str[INET_ADDRSTRLEN];
        //char wrt_buff[MAXLINE], rcv_buff[MAXLINE];

    int threads_count = 0;
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

                struct thread_info args;
                args.connfd = connfd;
                args.wrt_buff = wrt_buff;
                args.rcv_buff = rcv_buff;

                if (pthread_create(&threads[threads_count], NULL, request_func, (void *)connfd) != 0) {
                    printf("Error when creating thread %d\n", threads_count);
                    return 0;
                }

                if (++threads_count >= MAXTHREAD) {
                    break;
                }
                
        }
        printf("Nax thread number reached, wait for all threads to finish and exit...\n");
        for (int i = 0; i < MAXTHREAD; ++i) {
            pthread_join(threads[i], NULL);
        }

        return 0;
}

void* request_func(void *args)
{
    //struct thread_info *tinfo = args;
    int connfd, bytes_rcv, bytes_wrt, total_bytes_wrt;
    char wrt_buff[MAXLINE], rcv_buff[MAXLINE];

    FILE *file;
    char filename[128];
    int index = 0;

    /* get the thread info */
    //connfd = tinfo->connfd;
    //wrt_buff = tinfo->wrt_buff;
    //rcv_buff = tinfo->rcv_buff;
    int connfd = (int)args;
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
    sscanf(rcv_buff, "%s\n", filename);
    /* read from file */
    printf("Reading from file '%s' ...\n", filename);

    file = fopen(filename, "r");
    if (!file) {
                snprintf(wrt_buff, sizeof(wrt_buff) - 1, "%s\n", "The file your requested does not exist ...");
    } else {
        while ((wrt_buff[index] = fgetc(file)) != EOF) {
            ++index;
            }   
        fclose (file);
    }

    /* send response */
    bytes_wrt = 0;
    total_bytes_wrt = strlen(wrt_buff);
    while (bytes_wrt < total_bytes_wrt) {
            bytes_wrt += write(connfd, wrt_buff + bytes_wrt, total_bytes_wrt - bytes_wrt);
    }

    /* close the connection */
    close(connfd);
}
