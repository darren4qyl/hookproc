#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

using namespace std;

class Utils {
  public:
  Utils();
  ~Utils();
  void* getModuleBase(pid_t pid, const char* moduleName);
  void* getRemoteAddr(pid_t target_pid, const char* moduleName,
		      void* local_addr);
  void* getLibraryFuncAddr(const char* libraryPath, const char* funcName);
  void hookOperation(pid_t target_pid);

  private:
  int ptrace_attach(pid_t pid);
  int ptrace_getregs(pid_t pid, struct user_regs_struct* regs);
  int ptrace_setregs(pid_t pid, struct user_regs_struct* regs);
  int ptrace_continue(pid_t pid);
  int ptrace_call(pid_t pid, uint64_t addr, long* params, uint32_t num_params,
		  struct user_regs_struct* regs);
  int ptrace_writedata(pid_t pid, uint8_t* dest, uint8_t* data, size_t size);
  void print(const char* format, ...);
};

Utils::Utils() {}

Utils::~Utils() { cout << "~Utils() called" << endl; }

void* Utils::getModuleBase(pid_t pid, const char* moduleName) {
  char fileName[64];
  FILE* fp;
  long addr = 0;
  char buff[1024];
  memset(buff, 0, sizeof(buff));
  memset(fileName, 0, sizeof(fileName));
  if (pid <= 0) {
    // self process
    snprintf(fileName, sizeof(fileName), "/proc/self/maps");
  } else {
    snprintf(fileName, sizeof(fileName), "/proc/%d/maps", pid);
  }
  fp = fopen(fileName, "r");
  if (fp != NULL) {
    while (fgets(buff, sizeof(buff), fp)) {
      if (strstr(buff, moduleName)) {
	char* pch = strtok(buff, "-");
	addr = strtoul(pch, NULL, 16);
	if (addr == 0x8000) addr = 0;
	break;
      }
      memset(buff, 0, sizeof(buff));
    }
    fclose(fp);
  }
  return (void*)addr;
}

void* Utils::getRemoteAddr(pid_t target_pid, const char* moduleName,
			   void* local_addr) {
  void* local_handle = getModuleBase(-1, moduleName);
  void* remote_handle = getModuleBase(target_pid, moduleName);
  cout << "getRemoteAddr:local[" << local_handle << "] remote[" << remote_handle
       << "]" << endl;
  cout << "offset addr:"
       << ((void*)((uint64_t)local_addr - (uint64_t)local_handle)) << endl;
  return (void*)((uint64_t)local_addr - (uint64_t)local_handle +
		 (uint64_t)remote_handle);
}

void* Utils::getLibraryFuncAddr(const char* libraryPath, const char* funcName) {
  void* handle = dlopen(libraryPath, 2);
  if (handle == NULL) {
    cout << "dlopen error:" << strerror(errno) << endl;
    return (void*)-1;
  }
  void* symbol = dlsym(handle, "mmap");
  if (symbol == NULL) {
    cout << "dlsym error:" << strerror(errno) << endl;
    return (void*)-1;
  }
  return symbol;
}

void Utils::hookOperation(pid_t target_pid) {
  struct user_regs_struct tt, original_regs, maps;
  print("Suspend the target process");
  if (ptrace_attach(target_pid) == -1) {
    print("ERROR");
    return;
  }
  print("Get the target process register");
  if (ptrace_getregs(target_pid, &tt) == -1) {
    print("ERROR");
    return;
  }
  // save original register
  memcpy(&original_regs, &tt, sizeof(original_regs));

  long parameters[10];
  // set mmap parameters
  parameters[0] = 0;				       // addr
  parameters[1] = 0x4000;			       // size
  parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
  parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE;	 // flags
  parameters[4] = 0;				       // fd
  parameters[5] = 0;				       // offset

  void* mmap_addr =
      getLibraryFuncAddr("/lib/x86_64-linux-gnu/libc-2.23.so", "mmap");
  void* remote_mmap_addr = getRemoteAddr(
      target_pid, "/lib/x86_64-linux-gnu/libc-2.23.so", mmap_addr);

  // execute the mmap in target_pid
  if (ptrace_call(target_pid, (uint64_t)remote_mmap_addr, parameters, 6, &tt) == -1)
    return;

  print("operation======");
  // get the return values of mmap <in r0>
 // if (ptrace_getregs(target_pid, &maps) == -1) return;

  // get the start address for assembler code
}

int Utils::ptrace_attach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0) {
    cout << "ptrace_attach:" << strerror(errno) << endl;
    return -1;
  }
  waitpid(pid, NULL, WUNTRACED);
  if (ptrace(PTRACE_SYSCALL, pid, NULL, 0) < 0) {
    print("ptrace_attach:%s", strerror(errno));
    return -1;
  }
  waitpid(pid, NULL, WUNTRACED);
  return 0;
}

int Utils::ptrace_getregs(pid_t pid, struct user_regs_struct* regs) {
  if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
    print("ptrace_getregs:%s", strerror(errno));
    return -1;
  }
  return 0;
}

int Utils::ptrace_setregs(pid_t pid, struct user_regs_struct* regs) {
  if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
    perror("ptrace_setregs: Can not set register values");
    return -1;
  }
  return 0;
}

int Utils::ptrace_continue(pid_t pid) {
  if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
    perror("ptrace_continue:failed");
    return -1;
  }
  return 0;
}

/*
Func : 将size字节的data数据写入到pid进程的dest地址处
@param dest: 目的进程的栈地址
@param data: 需要写入的数据的起始地址
@param size: 需要写入的数据的大小，以字节为单位
*/
int Utils::ptrace_writedata(pid_t pid, uint8_t* dest, uint8_t* data,
			    size_t size) {
  long i, j, remain;
  uint8_t* laddr;
  size_t bytes_width = sizeof(long);

  //很巧妙的联合体，这样就可以方便的以字节为单位写入4字节数据，再以long为单位ptrace_poketext到栈中
  union u {
    long val;
    char chars[8];
  } d;

  j = size / bytes_width;
  remain = size % bytes_width;

  laddr = data;

  //先以4字节为单位进行数据写入

  for (i = 0; i < j; i++) {
    memcpy(d.chars, laddr, bytes_width);
    ptrace(PTRACE_POKETEXT, pid, dest, d.val);

    dest += bytes_width;
    laddr += bytes_width;
  }

  if (remain > 0) {
    //为了最大程度的保持原栈的数据，先读取dest的long数据，然后只更改其中的前remain字节，再写回
    d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);
    for (i = 0; i < remain; i++) {
      d.chars[i] = *laddr++;
    }

    ptrace(PTRACE_POKETEXT, pid, dest, d.val);
  }

  return 0;
}

int Utils::ptrace_call(pid_t pid, uint64_t addr, long* params,
		       uint32_t num_params, struct user_regs_struct* regs) {
  // put the first 4 parameters into %rdi，%rsi，%rdx，%rcx
  /*  regs->rdi = params[0];
    regs->rsi = params[1];
    regs->rdx = params[2];
    regs->rcx = params[3];
    regs->r8 = params[4];
    regs->r9 = params[5];*/

  print("sizeof=%d,%d", sizeof(long), sizeof(unsigned long));
  regs->rsp -= (num_params) * sizeof(long);
  ptrace_writedata(pid, (uint8_t*)&regs->rsp, (uint8_t*)params,
		   (num_params) * sizeof(long));

  long tmp_addr = 0x00;
  regs->rsp -= sizeof(long);
  ptrace_writedata(pid, (uint8_t*)&regs->rsp, (uint8_t*)&tmp_addr,
		   sizeof(tmp_addr));

  regs->rip = addr;

  /* // set the pc to func<e.g:mmap> that will be executed
   regs->rip = 0;

   // when finish this function,pid will stop
   regs->rbp = addr;

   long long unsigned int* p = &regs->rbp;
   p+=4;

   print("-----%p",p);
   *p=0;

   print("--------------- %p",&regs->rbp);*/
  // set the register and start to execute
  if (ptrace_setregs(pid, regs) == -1 || ptrace_continue(pid) == -1) {
    return -1;
  }

  print("=================");

  int stat = 0;
  waitpid(pid, &stat, WUNTRACED);
	print("---------------");
  while (stat != 0xb7f) {
		print("000000");
    if (ptrace_continue(pid) == -1) {
      printf("error\n");
      return -1;
    }
    print("999999");
    waitpid(pid, &stat, WUNTRACED);
  }

  // wait pid finish work and stop
//  waitpid(pid, NULL, WUNTRACED);

  print("11111111111111111");
  return 0;
}

void Utils::print(const char* format, ...) {
  va_list vArgList;
  char buffer[1024];
  va_start(vArgList, format);
  vsprintf(buffer, format, vArgList);
  cout << buffer << endl;
  va_end(vArgList);
}

int main(int argc, char** argv) {
  char op;
  pid_t target_pid = -1;
  while ((op = getopt(argc, argv, "t:")) != -1) {
    switch (op) {
      case 't':
	cout << "target pid:" << optarg << endl;
	target_pid = atoi(optarg);
	break;
      default:
	cout << "error" << endl;
	return 0;
	break;
    }
  }

  if (target_pid < 0) {
    cout << "parmeters illegal " << endl;
    return 0;
  }

  Utils* utils = new Utils();
  utils->hookOperation(target_pid);
  delete utils;
  return 1;
}
