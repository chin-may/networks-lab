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

int buffer_empty(int sent_loc, int acked_loc, int window_len){
    return sent_loc == acked_loc;
}

int buffer_full(int sent_loc, int acked_loc, int window_len){
    return (sent_loc + 1) % window_len == acked_loc;
}

int main(int argc, char *argv[]){
    srandom(time(NULL));
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
    int window_len = 10;
    while ((oopt = getopt(argc, argv, "dp:s:r:l:n:w:")) != -1){
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
            case 'w':
                window_len = atoi(optarg);
                break;
            case 'n':
                max_pack_num = atoi(optarg);
                break;
        }
    }
    char **window = malloc(window_len * sizeof(char*));
    int *tries = malloc(window_len * sizeof(int));
    struct timeval **sendtime = malloc(window_len * sizeof(struct timeval *)); 
    int i;
    for(i = 0; i < window_len; i++){
        sendtime[i] = malloc(sizeof(struct timeval));
    }
    int sent_loc = 0, acked_loc = 0;
    int ackednum = 0;
    if(fork() != 0){
        int transnum = 0;
        struct hostent *host = gethostbyname(addr);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(server_addr.sin_zero),8);


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

//      struct timeval timeout;
//      timeout.tv_usec = 0;
//      timeout.tv_sec = 0;
        int mu_timeout = 100000;

        struct timeval tval_zero;
        tval_zero.tv_usec = 0;
        tval_zero.tv_sec = 0;

        fd_set sockset;
        FD_ZERO(&sockset);
        FD_SET(sock, &sockset);

        ackednum = 0;
        int seqnum = 0;
        int i;
        char pbuf[sizeof(long)];
        unsigned long mutime, sectime;
        char *data = malloc(pack_len + 1);
        FILE *p_outptr = fdopen(p_out, "r");
        struct timeval *rec_time = malloc(sizeof(struct timeval));

        while(ackednum < max_pack_num){
            fd_set pipeset;
            FD_ZERO(&pipeset);
            FD_SET(lock[0], &pipeset);
            int pipe_sel_res = select(lock[0] + 1, &pipeset, NULL, NULL, &tval_zero);
            //printf("Pipe sel res: %d\n", pipe_sel_res);
            if( !buffer_full(sent_loc, acked_loc, window_len) && pipe_sel_res > 0){ //FIXME
                read(lock[0],pbuf, 1);
                read(p_out, pbuf, sizeof(long));
                mutime = 0;
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
                data = malloc(pack_len + 1);
                data[0] = seqnum;
                window[sent_loc] = data;
                sent_loc++;
                sent_loc %= window_len;
                sendto(sock, data, pack_len + 1, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
                printf("Sent seqnum %d\n", seqnum);
                transnum++;
                seqnum++;
                seqnum %= 127;

            }
            else{
                //printf("No packet gened\n");
            }


            //Now receiving acks
            FD_ZERO(&sockset);
            FD_SET(sock, &sockset);
            char ack_packet = 0;
            while(select(sock+1, &sockset, NULL, NULL, &tval_zero)){
                recvfrom(sock, buf, 1, 0, 
                        (struct sockaddr *)&client_addr, &addr_len);
                ack_packet = buf[0];
                printf("Got ack num %d\n", (int) ack_packet);
                int currloc = acked_loc;
                int found = 0;
                while(currloc != sent_loc){
                    if(window[currloc][0] == ack_packet){
                        found = 1;
                        break;
                    }
                    currloc++;
                    currloc %= window_len;
                }
                if(found){
                    int clearloc = acked_loc;
                    while(clearloc != (currloc + 1)%window_len){ 
                        free(window[currloc]);
                        window[currloc] = NULL;
                        clearloc++;
                        clearloc %= window_len;
                    }
                    acked_loc = currloc + 1;
                }

                FD_ZERO(&sockset);
                FD_SET(sock, &sockset);
            }

            //If anything timed out, resend from there
            int currloc = acked_loc;
            struct timeval currtime;
            gettimeofday(&currtime, NULL);
            if(!buffer_empty(sent_loc, acked_loc, window_len)){
                int difference = ( sendtime[currloc]->tv_sec - currtime.tv_sec ) *
                    1000000 + 
                    sendtime[currloc]->tv_usec - currtime.tv_usec;
                if(difference > mu_timeout){
                    while(currloc != sent_loc){
                        sendto(sock, window[currloc], pack_len + 1, 0, (struct sockaddr*) &server_addr, sizeof(struct sockaddr));
                        printf("Resending seqnum %d\n", (int) window[currloc][0]);
                        gettimeofday(sendtime[currloc], NULL);
                        transnum++;
                        currloc++;
                        currloc %= window_len;
                    }
                }
            }



//          FD_ZERO(&sockset);
//          FD_SET(sock, &sockset);
//          timeout.tv_usec = 100000;
//          int sel = select(sock+1, &sockset, NULL, NULL, &timeout);
//          if(sel < 1) {
//              //printf("timeout\n");
//              //fflush(stdout);
//              tries++;
//          }
//          else{
//              bytes_read = recvfrom(sock,buf,1024,0,
//                      (struct sockaddr *)&client_addr, &addr_len);
//              gettimeofday(rec_time, NULL);
//              ackednum++;
//              seqnum++;
//              seqnum %= 127;
//              if(debugc){
//                  printf("Seq #:%d; Time Generated: %lds %ldMus; Time ack Recd: %lds %ldMus; No of attempts: %d\n", seqnum,sectime%100, mutime, rec_time->tv_sec%100,rec_time->tv_usec, tries);
//              }
//              tries = 1;

//          }
        }
        gettimeofday(&end_time, NULL);
        double timediff = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec)/1000000.0;
        double mu_timediff = end_time.tv_usec - start_time.tv_usec;
        double throughput = (((double)(max_pack_num * pack_len * 8)) ) / (timediff * 1000);
        double mu_throughput = (((double)(max_pack_num * pack_len * 8)) ) / mu_timediff;
        float ter = ((float) transnum) / max_pack_num;
       
        if(timediff > 0)
            printf("PACKET_GEN_RATE: %d PACKET_LENGTH: %d Throughput: %lfKbps Transmission Efficiency Ratio: %f\n", pack_rate, pack_len, throughput, ter);
        else
            printf("PACKET_GEN_RATE: %d PACKET_LENGTH: %d Throughput: %lfMbps Transmission Efficiency Ratio: %f\n", pack_rate, pack_len, mu_throughput, ter);
        close(sock);
        return 0;
    }
    else{
        //printf("--forked\n");
        long avg_delay = 1000000000 / pack_rate;
        struct timespec *st = malloc(sizeof(struct timespec));
        st->tv_sec = 0;
        char out_buf[sizeof(long)];
        int i;
        struct timeval *gen_time = malloc(sizeof(struct timeval));
        FILE *p_inptr = fdopen(p_in, "w");
        int gennum = 0;
        while(gennum <= max_pack_num){
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
            write(p_in, out_buf, sizeof(long));
            for(i = 0; i < sizeof(long); i++){
                //out_buf[i] = (gen_time->tv_usec >> (sizeof(long) - (i + 1)*8)) & 0xFF;
                out_buf[i] = (gen_time->tv_sec >> (sizeof(long) - (i +1) )*8 ) & 0xff ;
            }
            gennum++;
            //printf("--about to write\n");
           // fprintf(p_inptr, "%ld",gen_time->tv_usec);
            //fflush(p_inptr);
            write(p_in, out_buf, sizeof(long));
            write(lock[1], out_buf, 1);
        }
        exit(0);
    }
}
