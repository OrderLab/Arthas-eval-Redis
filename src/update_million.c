#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data.txt", "w+");
        for(int i = 99999; i >= 0 ; i--){
                fprintf(fp, "SET key%d value%d\n", i, i);
        }
}
