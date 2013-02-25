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
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
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
    int dxxd = 1;
    char oopt;
    int pack_rate = 1;
    int debugc = 0;
    int pack_len;
    while ((oopt = getopt(argc, argv, "dp:s:r:l:n:")) != -1){
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
                debugc = 1;
                break;
            case 'l':
                pack_len = atoi(optarg);
                break;
            case 'n':
                max_pack_num = atoi(optarg);
                break;
        }
    }
    int ackednum = 0;
    if(fork() != 0){
        int transnum = 0;
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
        unsigned long mutime, sectime;
        char *data = malloc(pack_len + 1);
        int tries = 1;
        FILE *p_outptr = fdopen(p_out, "r");
        struct timeval *rec_time = malloc(sizeof(struct timeval));
        while(ackednum < max_pack_num){
            data[0] = seqnum;
            sendto(sock, data, pack_len + 1, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
            transnum++;
            //printf("Sent seqno %d\n", seqnum);
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
                gettimeofday(rec_time, NULL);
                ackednum++;
                //printf("Got ack %d\n", seqnum);
                seqnum = seqnum?0:1;
                tries = 1;
                //printf("debug: %d\n", debugc);
                //printf("max_pack_num: %d\n", max_pack_num);
                //printf("Seq #:%d; Time Generated: %ld; Time ack Recd: %ld; No of attempts: %d\n", seqnum, mutime, rec_time->tv_usec, tries);
                if(debugc){
                    printf("Seq #:%d; Time Generated: %ld; Time ack Recd: %ld; No of attempts: %d\n", seqnum, mutime, rec_time->tv_usec, tries);
                }
                //printf("About to block\n");
                read(lock[1],pbuf, 1);

                //printf("About to scan\n");
                //fscanf(p_outptr,"%ld",&mutime);
                read(p_out, pbuf, sizeof(long));
                //printf("Unblocked\n");
                mutime = 0;
                int i;
                for(i = 0; i < sizeof(long); i++){
                    mutime <<= 8;
                    mutime |= pbuf[i] & 0xff;
                }

                sectime = 0;
                read(p_out, pbuf, sizeof(long));
                for(i = 0; i < sizeof(long); i++){
                    sectime <<= 8;
                    sectime |= pbuf[i] & 0xff;
                }
                //printf("pipe out: %lx\n", mutime);
            }
        }
        gettimeofday(&end_time, NULL);
        double throughput = ((double)(max_pack_num * pack_len * 8)) / ( end_time.tv_sec - start_time.tv_sec );
        float ter = ((float) transnum) / max_pack_num;
        
        printf("PACKET_GEN_RATE: %d Packet length: %d Throughput: %lf Transmission efficiency ratio: %f\n", pack_rate, pack_len, throughput, ter);
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
        FILE *p_inptr = fdopen(p_in, "w");
        int gennum = 0;
        while(gennum < max_pack_num){
            st->tv_nsec = random()%(avg_delay*2 + 1);
            //printf("--about to sleep\n");
            nanosleep(st,NULL);
            //printf("--woke up\n");
            gettimeofday(gen_time, NULL);
            //printf("pipe in: %lx\n", gen_time->tv_usec);
            for(i = 0; i < sizeof(long); i++){
                //out_buf[i] = (gen_time->tv_usec >> (sizeof(long) - (i + 1)*8)) & 0xFF;
                out_buf[i] = (gen_time->tv_usec >> (sizeof(long) - (i +1) )*8 ) & 0xff ;
            }
            for(i = 0; i < sizeof(long); i++){
                //out_buf[i] = (gen_time->tv_usec >> (sizeof(long) - (i + 1)*8)) & 0xFF;
                out_buf[i] = (gen_time->tv_sec >> (sizeof(long) - (i +1) )*8 ) & 0xff ;
            }
            gennum++;
            //printf("--about to write\n");
           // fprintf(p_inptr, "%ld",gen_time->tv_usec);
            //fflush(p_inptr);
            write(p_in, out_buf, sizeof(long));
            write(lock[0], out_buf, 1);
        }
        exit(0);
    }
}
