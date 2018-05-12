//
// Created by 钱诚亮 on 2018/5/12.
//


#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include "API.h"
using namespace std;
class Interpreter{
public:

    API* ap;
    string fileName ;
    int interpreter(string s);

    string FetchWord(string s, int *st);

    Interpreter(){}
    ~Interpreter(){}
};

#endif