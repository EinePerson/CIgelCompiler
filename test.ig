int x = 4;
int y = 4;
x /= 2;
exit(x);
if(x == 6 && y == 4){
    if(false){
        if(false)exit(25);
        if(y == 4)exit(20);
        exit(10);
    }else if(x == 4){
        exit(15);
    }
}
exit(5);