#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv){
//0xd0e0f0a0;
        unsigned long mutime = 0xf0a0f0e0;
        printf("inp %lx\n", mutime);

        char out_buf[sizeof(long)];
        int i;
        for(i = 0; i < sizeof(long); i++){
            out_buf[i] = (mutime >> (sizeof(long) - (i +1) )*8 ) & 0xff ;
        }

        unsigned long outtime = 0;
        for(i = 0; i < sizeof(long); i++){
            outtime <<= 8;
            outtime |= out_buf[i] & 0xff;
        }

        printf("outp %lx\n", outtime);
        for(i = 0; i<sizeof(long); i++){
            printf("%x",out_buf[i]);
        }
        printf("\n");

        return 0;
}
