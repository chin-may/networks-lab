#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

ssize_t unrel_recvfrom(int sockfd, void *buf, size_t len, int flags,
        struct sockaddr *src_addr, socklen_t *addrlen, float probab){
    int probab_int = (int) (probab*100);
    int r = random() % 100;
    while(r < probab_int){
        char* tmp = malloc(len);
        recvfrom(sockfd, tmp, len, flags, src_addr, addrlen);
        free(tmp);
        r = random() % 100;
    }
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

}

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
    float probab = 0.5;
    int max_pack_num = 100;
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

    int exp_seq = 0;
    int ackednum = 0;

    long start_time, ack_time;

    while (ackednum < max_pack_num){

        bytes_read = unrel_recvfrom(sock,recv_data,1024,0,
                (struct sockaddr *)&server_addr, &addr_len, probab);

        if(bytes_read[0] == exp_seq){
            send_data[0] = exp_seq;
            sendto(sock, send_data, 1, 0,
                    (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

            exp_seq = exp_seq?0:1;
            ackednum++;
        }
    }

    close(sock);
    return 0;
}
