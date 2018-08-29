#ifndef _FORK_HPP_DEFINED_
#define _FORK_HPP_DEFINED_

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <ctype.h>
#include <sys/types.h>
#include <ext/stdio_filebuf.h>

class forker_t
{
    // child to parent FD
    int fdc2p[2];

    int rdfd;
    int wrfd;

    // pid of the child.
    pid_t childpid;

public:
    forker_t() {
        if(pipe(fdc2p) == -1) {
            std::cerr << "Unable to create pipe." << std::endl;
            exit(1);
        }            
    }
    ~forker_t() {
        close();
    }

    ssize_t read(void* ptr, ssize_t bytes);
    ssize_t write(const void* ptr, ssize_t bytes);
    void close() { ::close(wrfd); ::close(rdfd); }

    pid_t fork();
};


#endif
