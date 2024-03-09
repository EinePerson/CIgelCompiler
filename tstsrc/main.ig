namespace Igel{

    /*struct Test{
        Test t;
        int a;
        int b;
        //int[] arr;
    }*/

    class Gaming{
        int i = 0;
        int j = 5;

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
}

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
    Igel::Gaming g = new Igel::Gaming();
    g->i = 64;
    printf("%d",g->i);
    printf("%d",g->j);
    g->sus();
    printf("%d",g->i);
    return 0;
}
