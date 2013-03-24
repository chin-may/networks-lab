#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

int main(int argc, char** argv){

    int user_num = 10;
    double ber = 0;
    int packet_len = 10;
    double prob_b = 0.5, prob_n = 1;
    double gen_rate = 0.01;
    int max_pack_num = 100;
    int debug = 0;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);




    double gen_prob = 1 - pow(M_E, -gen_rate);
    double pack_err_rate = 1 - pow(1-ber, 8*packet_len);

    int *intent = malloc(user_num * sizeof(int));
    int *is_backed = malloc(user_num * sizeof(int));
    int *fail_count = malloc(user_num * sizeof(int));
    int *seq_num = malloc(user_num * sizeof(int));
    double *prev_prob = malloc(user_num * sizeof(double));
    struct timeval **gen_time = malloc(user_num * sizeof(struct timeval*));

    int i;

    int success_num = 0;
    int slot_num = 0;

    srand(time(NULL));

    for(i = 0; i < user_num; i++){
        prev_prob[i] = prob_b;
        seq_num[i] = 0;
        fail_count[i] = 0;
        is_backed[i] = 0;
        gen_time[i] = malloc(sizeof(struct timeval));
    }

    while(1){
        slot_num++;
        //sleep(1);
        printf("next\n");
        for(i = 0; i < user_num; i++){
            intent[i] = 0;
        }

        //For backlogged senders
        for(i = 0; i < user_num; i++){
            if(is_backed[i]){
                if(random()%100 < prev_prob[i]*100){
                    //printf("Back %d\n",i);
                    intent[i] = 1;
                }
            }
        }

        //for new generation
        for(i = 0; i < user_num; i++){
            int r = random()%100;
            if(!is_backed[i] && r < gen_prob * 100){
                //printf("new %d rand: %d  geNp: %lf \n",i,r,gen_prob);
                gettimeofday(gen_time[i],NULL);
                if(random()%100 < prob_n * 100){
                    intent[i] = 1;
                }
                else{
                    is_backed[i] = 1;
                }
            }
        }


        //Checking for collision
        int blocked = 0;
        for(i = 0; i < user_num; i++){
            if(intent[i]){
                blocked++;
            }
        }

        //Collision occured
        if(blocked > 1){
            printf("Collision between ");
            for(i = 0; i < user_num; i++){
                if(intent[i]){
                    is_backed[i] = 1;
                    prev_prob[i] = max(0.1, prev_prob[i] / 5);
                    fail_count[i]++;
                    printf(" %d,",i);
                }
            }
            printf("\n");
        }
        else if(blocked == 1){
            for(i = 0; i < user_num; i++){
                if(intent[i]){
                    if(random()%100 >= pack_err_rate * 100){
                        is_backed[i] = 0;
                        prev_prob[i] = min(0.5, prev_prob[i] * 2);
                        printf("Successfully sent seqnum:%d by %d number of attempts:%d\n"
                                , seq_num[i], i,fail_count[i]+1);
                        struct timeval sent_time;
                        gettimeofday(&sent_time, NULL);
                        printf("Seq #:%d Time Generated:%ds %dmus Time Sent: %ds %d mus Number of attempts %d",
                                seq_num[i], gen_time[i]->tv_sec - start_time.tv_sec,
                                gen_time[i]->tv_usec - start_time.tv_usec,
                                sent_time.tv_sec - gen_time[i]->tv_sec,
                                sent_time.tv_usec - gen_time[i]->tv_usec,
                                fail_count[i]+1);
                        success_num++;
                        seq_num[i]++;
                        fail_count[i] = 0;
                    }
                    else{
                        is_backed[i] = 1;
                        prev_prob[i] = max(0.1, prev_prob[i] / 5);
                        fail_count[i]++;
                        printf("Packet error %d\n",i);
                    }
                }
            }
        }
        if(success_num >= max_pack_num) break;
    }

    printf("\nUtilisation: %lf\n",((double)success_num)/slot_num);

    return 0;
}
