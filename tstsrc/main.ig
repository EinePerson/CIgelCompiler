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
    ABC t = new ABC;
    t.a = 26;
    printf("%d",t.a);
    int[] t1 = new int[5];
    t1[0] = 254;
    byte l = 25;
    printf("%d",t1[0]);
    printf("%d",l);
    test(t);
    return 5;
}

void test(ABC i){
    printf("%d",i.a);
}
