
#include <string>
#include <thread>
#include <iostream>
#include "TCBfr.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Argument error" << endl;
    }
    while (true) {
        //cout<<"New connection"<<endl;
        TCB tcb(string(argv[2]), atoi(argv[1]));
        tcb.init();
        cout << "Listening on Port: " << tcb.portNum << endl;
        cout << "File length: " << tcb.fileLen << endl;
        tcb.accept();
        tcb.send();
        // cout << "send finish" << endl;
        tcb.tear_down();
        // cout << "tear down" << endl;
    }
}
