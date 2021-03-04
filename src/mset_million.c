#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data.txt", "w+");
	fprintf(fp, "MSET ");
        for(int i = 0; i < 10000; i++){
                fprintf(fp, "key%d value%d ", i, i);
        }
}

