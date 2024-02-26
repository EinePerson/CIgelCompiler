/*namespace Igel{

    struct Test{
        Test t;
        int a;
        int b;
        //int[] arr;
    }

    void abc(){

    }

    class Gaming{
        void test(){

        }
    }
}*/

struct Amogus{
    int test;
    int gaming;
}

struct ABC{
    static int c = 23;
    static Amogus sus = new Amogus;
    int a;
}

int main(){
    Amogus a = new Amogus;
    a.gaming = 25;
    a.test = 76;
    printf("%d",a.gaming);
    ABC::sus.test = 253;
    ABC::sus.gaming = 84;
    printf("%d",ABC::sus.gaming);
    ABC::c = 98;
    return ABC::c;
}
