int x = 4;
int y = 4;
if(x == 4 && x != 3){
    if(y <= 3){
        if(y != 4)exit(20);
        exit(10);
    }else if(y >= 3){
        exit(15);
    }else exit(25);
}
exit(5);