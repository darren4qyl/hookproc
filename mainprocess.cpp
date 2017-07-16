#include <iostream>
#include <unistd.h>

using namespace std;

static char* TAG = (char*)"global tag";

int main(){
    cout << "my pid:" << getpid() << endl;
    while(1){
       sleep(100);
       cout << TAG << ":out input val===;" << endl;
    }
    return 1;
}
