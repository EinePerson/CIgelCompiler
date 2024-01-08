struct Test{
    Test t;
    int a;
}

int main(){
    Test test = new Test;
    test.t = new Test;
    test.t.a = 3;
    test.t.a *= 3;
    test.t.b = 8;

    exit(test.t.a);
    exit(50);
    return 5;
}