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
    srandom(time(NULL));
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
        perror("socket failed");
        exit(1);
    }
    struct sockaddr_in server_addr, client_addr;
    int port = 6000;
    float probab = 0.2;
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
    while ((oopt = getopt(argc, argv, "p:n:e:")) != -1){
        switch(oopt){
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                max_pack_num = atoi(optarg);
                break;
            case 'e':
                probab = atof(optarg);
        }
    }
    printf("%d\n", port);

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

    char send_data[1024], recv_data[1024];
    int bytes_read;

    int addr_len = sizeof(struct sockaddr);

    int exp_seq = 0;
    int ackednum = 0;

    long start_time, ack_time;
    int corrupted = 0;
    struct timeval recd_time;

    while (ackednum < max_pack_num){

        bytes_read = recvfrom(sock,recv_data,1024,0,
                (struct sockaddr *)&client_addr, &addr_len);
        gettimeofday(&recd_time,NULL);
        if(random()%1000000 < (probab*1000000)){
            corrupted = 1;
            printf("Seq #:%d; Time Received: %lds %ldMus Corrupted: Yes; Accepted: No\n", recv_data[0], recd_time.tv_sec%100, recd_time.tv_usec);
        }
        else{
            corrupted = 0;
            printf("Seq #:%d; Time Received: %lds %ldMus Corrupted: No; Accepted: ", recv_data[0], recd_time.tv_sec%100, recd_time.tv_usec);

            if(recv_data[0] == exp_seq){
                printf("Yes\n");
                send_data[0] = exp_seq;
                sendto(sock, send_data, 1, 0,
                        (struct sockaddr *)&client_addr, sizeof(struct sockaddr));

                exp_seq = exp_seq?0:1;
                ackednum++;
            }
            else{
                printf("No\n");
            }
        }
    }

    close(sock);
    return 0;
}
