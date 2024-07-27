//
// Created by igel on 20.07.24.
//

#ifndef TEST_H
#define TEST_H

#include <stdio.h>

namespace SUS{
    struct Gaming{
      public:
        Gaming() {

        }

        virtual ~Gaming() = default;

        int k = 0;
        virtual void test() {
			printf("%d\n",k);
        }
    };
}

class Amogus {
  	Amogus() __attribute__((used)){
    }
	virtual void sus() = 0;
};

/*class Test : public SUS::Gaming,public Amogus{
public:
  	Test(){

    }

    int i;
    int j;

    void test() override {
    	printf("%dABC\n",k);
    }

    void sus() override {
    	printf("Gaming\n");
    }
};*/

SUS::Gaming* test(SUS::Gaming* g){
  	delete new SUS::Gaming();
    //new Test();
    printf("%d\n",g->k);
    //g->k = 420;
    g->test();
    return g;
}

#endif //TEST_H
