int main(){
    SUS s;
    s.a = 50;
    s.b = 69;
    s.c = 420;
    Gaming t = new Gaming();
    t.j = 69;
    printf("%d\n",t.i);
    Test g = null;
    //SIGSEGV to check if debug info works
    g.i = 69;
    return 0;
}

struct SUS{
    int a;
    int b;
    int c;
}

class Test{
    int i;

    void tst(){

    }

    Test(){
        this.i = 27;
    }
}

class Gaming extends Test{
    int j;
    int k;
}