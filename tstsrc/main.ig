int main(){
    Amogus b;
    b.test = 101;
    Gaming g = new Gaming();
    g.i = 64;
    Gaming[][] a = new Gaming[10][5];
    a[0][0] = new Gaming();
    a[0][0].i = 53;
    printf("%d",a[0][0].i);
    printf("%d",a.length);
    return 0;
}


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
        this.i++;
        printf("%d",40);
    }
}

struct Amogus{
    int test;
    int gaming;
}