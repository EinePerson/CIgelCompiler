int main(){
    SUS a = new SUS();
    printf("Test 1\n");
    printf("%d\n",a.getSUS(74));
    printf("%d\n",a.c);
    a.amo = new Amogus();
    a.amo.e = 420;

    int i = false?26:73;
    printf("%d",i);

    a.c = 15;
    a.b = 69;
    printf("Test3\n");
    a.a();
    printf("%d\n",a.c);
    a.test();
    printf("%d\n",a.b);
    printf("%d\n",a.amo.e);
    a.gab();

    return 0;
}

class Amogus{

    /*Amogus(){

    }*/

    int d;
    int e;
    int f;
}

class SUS extends Gaming{
    int c;
    long b;
    Amogus amo;

    SUS(){
        super(10);
        printf("SUSY Gaming\n");
    }

    void test(){
        this.c = 69;
        printf("Test\n");
    }

    override void a(){
        printf("Gaming\n");
    }

    override void gab(){
        printf("SUSY Amogus\n");
    }
}

abstract class Gaming implements abc{

    Gaming(int i){
        printf("Test2\n");
    }

    int getSUS(int i){
        return i;
    }

    abstract void a();

}

interface abc{
    void gab();
}

interface susy{
    int getSUS(int i);
}

/*struct Amogus{
    int test;
    int gaming;
}*/