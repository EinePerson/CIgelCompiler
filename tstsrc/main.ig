struct Test{
    Test t;
    int a;
    int b;
    int[] arr;
}

int main(){
    Test te = new Test;
    te.t = new Test;
    te.t.a = 364;
    printf("%d",te.t.a);
    Test[] t = new Test[5];
    t[2] = new Test;
    t[2].a = 264;
    t[0] = new Test;
    t[0].a = 51;
    printf("%d",t[2].a);
    return 5;
}