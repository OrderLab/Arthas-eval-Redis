
#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data3.txt", "w+");
        for(int i = 0; i < 1000000; i++){
                fprintf(fp, "DEL key%d\n", i);
        }
}
