int main(){
    final SUS a = new SUS();
    a.c = 69420;

    try{
        throw a;
    }catch(SUS excp){
        printf("%d\n",excp.c);
    }

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