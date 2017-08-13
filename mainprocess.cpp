#include <iostream>
#include <unistd.h>

using namespace std;

static char* TAG = (char*)"global tag";

int main(){
    cout << "my pid:" << getpid() << endl;
    unsigned long count = 0;
    while(1){
       sleep(1);
       count++;
       cout << TAG << ":out input val===" << count << endl;
    }
    return 1;
}
