#include <stdio.h>
#include <dlfcn.h>

const char* _dlopen_param1_s = "test.so";
int countf = 0; //rffds

int main(){
  void *addr = dlopen(_dlopen_param1_s,2);
  printf("hello world\n");
  return 1;
}
