#include <stdio.h>

int main(){
        FILE *fp;
        fp = fopen("data_long.txt", "w+");
        for(int i = 0; i < 100; i++){
                fprintf(fp, "SET key%d PMd7iqXN0i7PfzuDLlo3mQXObpCI5S93VVvbhhJzgR7yuVLVHt9tXKJYtKBDiPUmY8amzseEVYvoa8HTLvlzDAEIpiZfWbiQ5vxIgS9ZBJFnHhwX88eckYvFhvzFd1RZO5zPyUw9xqODdsjk2pyW8GuonyxJDJrc0YzBRx1LH58NydavNfWna87jiNJiOom19fQ6cezibuGyUl8doG4XrJPlpccmhbjEwuBLHv9o4CFso0D2tk4d2ClHmD6KV094x0cne5vlXXpBIXWtR9lAy1yhQEwrR78gfPrbPfHUdZrefU66Xcuu0rLjsxYjyFePp0Esh3Mhk7qerSBZCwzMaCy9foU0CStjbxmtS6q0amoRz8jh1bbXqtnedT8Nn453G5r1hZsWfCwosBRX\n", i);
        }
}

