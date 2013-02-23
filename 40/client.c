#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char** argv){
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        perror("socket failed");
        exit(1);
    }
    struct sockaddr_in server_addr, client_addr;
    int port = 6000;
    char addr[80];
    //char opt = getopt(argc, argv, "ap");
    //if(opt = 'p')
        //port = atoi(optarg);
    //else 
        //strncpy(addr, optarg, 80);
    //opt = getopt(argc, argv, "ap");
    //if(opt = 'p')
        //port = atoi(optarg);
    //else 
        //strncpy(addr, optarg, 80);

    //printf("%s:%d\n", addr, port);

    char oopt;
    while ((oopt = getopt(argc, argv, "a:p:")) != -1){
        switch(oopt){
            case 'a':
                strncpy(addr,optarg,80);
                break;
            case 'p':
                port = atoi(optarg);
        }
    }
    printf("\n%s:%d\n", addr, port);
    struct hostent *host = gethostbyname(addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);
    char send_data[1024], recv_data[1024];
    int bytes_read;

    int addr_len = sizeof(struct sockaddr);

    fd_set sockset;
    FD_ZERO(&sockset);
    FD_SET(sock, &sockset);

    int opts = fcntl(sock, F_GETFL);
    if(opts < 0){
        perror("getfl\n");
        exit(1);
    }
    opts = opts | O_NONBLOCK;
    if(fcntl(sock,F_SETFL, opts) < 0){
        perror("set nonblock failed");
        exit(1);
    }

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int sel;


    while (1){

        printf("Enter roll number:\n");
        scanf("%s",send_data);

        if ((strcmp(send_data , "q") == 0) || strcmp(send_data , "Q") == 0)
            break;

        sendto(sock, send_data, strlen(send_data), 0,
                (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
        FD_ZERO(&sockset);
        FD_SET(sock, &sockset);

        sel = select(sock+1, &sockset, NULL, NULL, &timeout);
        while(sel < 1) {
            printf("No response\n");
            sendto(sock, send_data, strlen(send_data), 0,
                    (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
            FD_ZERO(&sockset);
            FD_SET(sock, &sockset);

            sel = select(sock+1, &sockset, NULL, NULL, &timeout);
        }
        bytes_read = recvfrom(sock,recv_data,1024,0,
                (struct sockaddr *)&server_addr, &addr_len);

        recv_data[bytes_read] = '\0';
        printf("%s\n", recv_data);
    }

    close(sock);



    return 0;
}
