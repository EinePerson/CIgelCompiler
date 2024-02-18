/*namespace Igel{

    struct Test{
        Test t;
        int a;
        int b;
        //int[] arr;
    }

    void abc(){

    }

}*/

struct ABC{
    int a;
}

int main(){
    int[] a = new int[5];
    a[0] = 25;
    a[3] = 2;
    printf("%d",a[0]);

    ABC b = new ABC;
    b.a = 554;
    printf("%d",b.a);

    ABC[] arr = new ABC[5];
    arr[0] = new ABC;
    arr[0].a = 256;
    printf("%d\n",arr[0].a);
    return 5;
}
