int main(){
    SUS a = new SUS();
    a.a = 69;
    try{
        throw a;
    }catch(SUS c){
        printf("%d\n",c.a);
    }

    return 0;
}


class SUS{
    int a;
}

class Gaming{

}

/*struct Amogus{
    int test;
    int gaming;
}*/