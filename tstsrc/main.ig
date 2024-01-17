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
    while(test.a > 10){
        if(test.a == 10){
            break;
        }
        test.a--;
    }
    /*switch(test.a){
        case 7:
            printf("C");
            break;
        case 10:
            printf("A");
            break;
        case 5:
            printf("B");
            break;
        case 2:
            printf("D");
            break;
        default:
            printf("E");
            break;
    }*/
    return 5;
}