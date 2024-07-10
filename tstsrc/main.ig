using "tstsrc/exception.ig";
using "tstsrc/string.ig";

int main(){
    Test t = new SUS();
    printf("Test\n");
    if(t instanceOf SUS)printf("SUS");
    SUS g = (SUS) t;
    return 0;
}

class Test{

}

class SUS extends Test{

}

class Gaming extends Test{

}

/*class Exception {
    int i = 0;
    int j = 0;
    void print(){

    }
}*/

/*interface Test{

}*/