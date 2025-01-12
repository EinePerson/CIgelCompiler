using "tstsrc/test.ig";

class Gaming{
    int i;
    int j;
}

class Sus{
    int z;
}

class Test<T>{
    T amogus;
    void test(){

    }

    T get(){
        return this.amogus;
    }
}

int main(){
    Test<Gaming> g = new Test<Gaming>();
    g.amogus = new Gaming();
    g.amogus.i = 69;
    Gaming a = g.get();
    printf("%d",a.i);
    return 0;
}