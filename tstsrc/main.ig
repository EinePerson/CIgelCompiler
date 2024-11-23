using "tstsrc/test.ig";

class Test{
    void test(){

    }
}

int main(){

    int[] arr = new int[5];
    arr[4] = 10;
    Test t;
    printf("%p,%lu,%d\n",arr,arr.length,arr[4]);
    t.test();

    arr[5] = 69;
    /*int i = 25;
    int j = 3;
    printf("%d",i % j);
    printf("ABC");*/
    return 0;
}