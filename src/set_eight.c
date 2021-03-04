#include <stdio.h>

int main(){
        FILE *fp[8];
        fp[0] = fopen("data0.txt", "w+");
        fp[1] = fopen("data1.txt", "w+");
        fp[2] = fopen("data2.txt", "w+");
        fp[3] = fopen("data3.txt", "w+");
        fp[4] = fopen("data4.txt", "w+");
        fp[5] = fopen("data5.txt", "w+");
        fp[6] = fopen("data6.txt", "w+");
        fp[7] = fopen("data7.txt", "w+");
	for(int i = 0; i < 8; i++){
		for(int j = i*1250000; j < (i+1)*1250000; j++){
			fprintf(fp[i], "SET key%d value%d\n", j, j);
		} 


	}


}


