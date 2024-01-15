struct Test{
    Test t;
    int a;
    int b;
}

double test(double d){
    return d * d;
}

int main(){
    Test test = new Test;
    test.a = 12;
    test.a--;
    test.a--;
    test.a -= 5;
    switch(test.a){
        case 10:
            printf("A");
            break;
        case 5:
            printf("B");
            break;
    }
    return 5;
}