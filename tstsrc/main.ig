//namespace Igel{

    /*struct Test{
        Test t;
        int a;
        int b;
        //int[] arr;
    }*/

    class Gaming{
        int i = 0;
        int j = 0;

        void gaming(){
            printf("%d",23);
        }

        void test(){
            printf("%d",53);
        }

        void sus(){
            this->i++;
            printf("%d",40);
        }
    }
//}

struct Amogus{
    int test;
    int gaming;
}

/*struct ABC{
    static int c = 23;
    //static Amogus sus = new Amogus;
    int a;
}*/

int main(){
    Gaming[] a = new Gaming[5];
    a[0] = new Gaming();
    a[0]->i = 73;
    printf("%d",a[0]->i);
    return 0;
}
