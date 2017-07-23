#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    cout << "getRemoteAddr:local[" << local_handle << "] remote["
         << remote_handle << "]" << endl;
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
    struct user_regs_struct tt;
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
    void* mmap_addr =
        utils->getLibraryFuncAddr("/lib/x86_64-linux-gnu/libc-2.23.so", "mmap");
    void* remote_mmap_addr = utils->getRemoteAddr(
        target_pid, "/lib/x86_64-linux-gnu/libc-2.23.so", mmap_addr);
    cout << "remote mmap address:" << remote_mmap_addr << endl;

    utils->hookOperation(target_pid);
    delete utils;
    return 1;
}
