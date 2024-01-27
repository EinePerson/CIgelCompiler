struct Test{
    Test t;
    int a;
    int b;
    //int[5] arr;
}

int main(){
    //Test t = new Test;
    int[][] arr = new int[5][5];
    arr[2][3] = 10;
    arr[2][3] = 11;
    //arr[2][1] = 9;
    printf("%d",arr[2][3]);
    return 5;
}