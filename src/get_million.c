#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data2.txt", "w+");
        for(int i = 0; i < 100000; i++){
                fprintf(fp, "GET key%d\n", i);
        }
}
