
#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data_unlink.txt", "w+");
        for(int i = 0; i < 100; i++){
                fprintf(fp, "UNLINK key%d\n", i);
        }
}

