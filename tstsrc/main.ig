include "includeTest/test.h";

int main(){
    SUS::Gaming g = new SUS::Gaming();
    g.k = 69;
    g.test();
    printf("DEF\n");
    g.k = 580;
    test(g);
    Test t = new Test();
    t.sus();
    t.k = 18;
    t.i = 50;
    t.j = 420;
    printf("%d\n",t.k);
    printf("ABC");
    return 0;
}

class Test extends SUS::Gaming implements Amogus{
    int i;
    int j;

    override void test() {
    	printf("%dABC\n",this.k);
    }

    override void sus()  {
        	printf("Gaming\n");
    }
};