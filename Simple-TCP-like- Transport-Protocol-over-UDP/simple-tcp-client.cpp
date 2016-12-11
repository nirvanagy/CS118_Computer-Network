#include <string>
#include <thread>
#include <iostream>
#include <stdio.h>
#include "clientHelper.h"

int main(int argc, const char *argv[])
{
    if (argc < 3) {
        std::cerr << "./client SERVER-HOST-OR-IP PORT-NUMBER" << std::endl;
        exit(1);
    }


    Client cl;

    cl.init(argv[1], atoi(argv[2]));
    cl.connect();
    cl.receive();
    cl.finish();
}