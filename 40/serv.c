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
#include <fcntl.h>

int main(int argc, char *argv[]){
    int pipe_fd[2];
    int lock[2];
    pipe(pipe_fd);
    pipe(lock);
    int p_out = pipe_fd[0], p_in = pipe_fd[1];

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
    char addr[80];
    int max_pack_num = 100;
    char oopt;
    int pack_rate = 1;
    int debug = 0;
    int pack_len;
    while ((oopt = getopt(argc, argv, "p:a:r:")) != -1){
        switch(oopt){
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                strncpy(addr,optarg, 80);
                break;
            case 'r':
                pack_rate = atoi(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            case 'l':
                pack_len = atoi(optarg);
            case 'n':
                max_pack_num = atoi(optarg);
        }
    }
    int ackednum = 0;
    if(fork() == 0){
        struct hostent *host = gethostbyname(addr);
        printf("Connecting to %s %d\n",addr, port);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8);


        printf("Client started\n");
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
        timeout.tv_sec = 0;
        fd_set sockset;
        FD_ZERO(&sockset);
        FD_SET(sock, &sockset);

        ackednum = 0;
        int seqnum = 0;
        char pbuf[sizeof(long)];
        long mutime;
        char data[80];
        int tries = 0;
        while(ackednum < max_pack_num){
            data[0] = seqnum;
            sendto(sock, data, 80, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
            printf("Sent seqno %d\n", seqnum);
            FD_ZERO(&sockset);
            FD_SET(sock, &sockset);
            timeout.tv_usec = 100000;
            int sel = select(sock+1, &sockset, NULL, NULL, &timeout);
            if(sel < 1) {
                printf("timeout\n");
                fflush(stdout);
                tries++;
            //    if(tries > 15) exit(1);
            }
            else{
                bytes_read = recvfrom(sock,buf,1024,0,
                        (struct sockaddr *)&client_addr, &addr_len);
                ackednum++;
                printf("Got ack %d\n", seqnum);
                seqnum = seqnum?0:1;
                //printf("About to block\n");
                read(lock[1],pbuf, 1);

                read(p_out, pbuf, sizeof(long));
                //printf("Unblocked\n");
                mutime = 0;
                int i;
                for(i = 0; i < sizeof(long); i++){
                    mutime |= pbuf[i];
                    mutime <<= 1;
                }
            }
        }
        close(sock);
        return 0;
    }
    else{
        printf("--forked\n");
        long avg_delay = 1000000000 / pack_rate;
        struct timespec *st = malloc(sizeof(struct timespec));
        st->tv_sec = 0;
        char out_buf[sizeof(long)];
        int i;
        struct timeval *gen_time = malloc(sizeof(struct timeval));
        while(ackednum < max_pack_num){
            st->tv_nsec = random()%(avg_delay*2 + 1);
            //printf("--about to sleep\n");
            nanosleep(st,NULL);
            //printf("--woke up\n");
            gettimeofday(gen_time, NULL);
            for(i = 0; i < sizeof(long); i++){
                out_buf[i] = (gen_time->tv_usec << (sizeof(long) - i));
            }
            //printf("--about to write\n");
            write(p_in, out_buf, sizeof(long));
            write(lock[0], out_buf, 1);
        }
    }
}
