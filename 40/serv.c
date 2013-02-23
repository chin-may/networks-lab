#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "BSTree.h"



int main(int argc, char *argv[]){
    int pipe_fd[2];
    int p_out = fd[0], p_in = fd[1];

    pipe(pipe_fd);

    if(fork() == 0){
        int sock;
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock == -1){
            perror("socket failed");
            exit(1);
        }
        struct sockaddr_in server_addr, client_addr;

        //getopt(argc, argv, "p:");
        //int port = atoi(optarg);
        int port = 6000;
        int max_pack_num = 100;
        char oopt;
        while ((oopt = getopt(argc, argv, "p:")) != -1){
            switch(oopt){
                case 'p':
                    port = atoi(optarg);
            }
        }
        printf("Starting server on %d\n", port);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(server_addr.sin_zero),8);

        if (bind(sock,(struct sockaddr *)&server_addr,
                    sizeof(struct sockaddr)) == -1)
        {
            perror("Bind");
            exit(1);
        }

        printf("\nUDPServer Waiting for client on port %d\n", port);
        fflush(stdout);

        int bytes_read;
        char buf[1024];

        int addr_len = sizeof(struct sockaddr);
        int seq_no = 0;

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
        timeout.tv_usec = 0;
        timeout.tv_sec = 1;
        fd_set sockset;
        FD_ZERO(&sockset);
        FD_SET(sock, &sockset);

        int ackednum = 0;
        while(ackednum < max_pack_num){
            buf[bytes_read] = '\0';
            printf("recd %s\n",buf);
            sendto(sock, name, strlen(name), 0, (struct sockaddr*) &client_addr, sizeof(struct sockaddr));
            FD_ZERO(&sockset);
            FD_SET(sock, &sockset);

            int sel = select(sock+1, &sockset, NULL, NULL, &timeout);
            if(sel < 1) {
                printf("timeout\n");
                fflush(stdout);
                continue;
            }
            bytes_read = recvfrom(sock,buf,1024,0,
                    (struct sockaddr *)&client_addr, &addr_len);
        }
        close(sock);
        return 0;
    }
    else{

    }
}
