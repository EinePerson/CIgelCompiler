struct Test{
    Test t;
    int a;
    int b;
    int[] arr;
}

int main(){
    Test test = new Test;
    test.a = 164;
    test.arr = new int[5];
    test.arr[2] = 136;
    test.arr[0] = 137;
    test.arr[3] = 138;
    printf("%d",test.a);
    return 5;
}