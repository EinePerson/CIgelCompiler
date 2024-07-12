using "tstsrc/exception.ig";
using "tstsrc/string.ig";

int main(){
    SUS s = new SUS();
    //s.a = 69;
    s.b = 420;
    //s.c = 283;
    printf("%d\n",s.c);
    //printf("%d\n",s.a);
    printf("%d\n",s.b);
    s.test();
    //printf("%d\n",s.a);
    return 0;
}

class Gaming{
    protected int a;
    int b;

    void amogus(){
        this.a = 151;
    }

}

class SUS extends Gaming{
    int c;

    public void test(){
        int test = 0;
        this.amogus();
    }

    override void amogus(){
        this.a = 125;
    }
}