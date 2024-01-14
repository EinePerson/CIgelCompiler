struct Test{
    Test t;
    int a;
    int b;
}

double test(double d){
    return d * d;
}

int main(){
    int i = 51;
    double d = 10.5D;
    printf("%f",i / d);
    Test test = new Test;
    test.t = new Test;
    test.t.a = 3;
    test.t.a *= 3;
    test.t.b = 8;
    if(test.t.b == 8)printf("SGS");
    int i = 0;
    printf("A");
    while(i < 10){
        if(i == 8){
            printf("Test");
            break;
        }
        i++;
        if(i == 5)continue;
        printf("%d",i);
    }
    if(test.t.b == 8){
        if(test.t.a == 9)printf("ABCD");
    }
    printf("%f",test(5.05D));
    exit(test.t.b);
    exit(50);
    return 5;
}