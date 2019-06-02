#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <ext/stdio_filebuf.h>

#include "forker.h"

ssize_t forker_t::read(void* ptr, ssize_t total_bytes)
{
    // number of bytes remaining to be read.
    ssize_t bytes_remaining = total_bytes;
    do {
        // attempt to read all bytes remaining.
        ssize_t bytes_read = ::read(rdfd, ptr, bytes_remaining);
        if(bytes_read == 0) {
            // eof.
            return 0;
        } else if(bytes_read == -1) {
            // error.
            return -1;
        } else {
            // update bytes_remaining.
            bytes_remaining -= bytes_read;
        }
    } while(bytes_remaining > 0);
    return total_bytes;
}

ssize_t forker_t::write(const void* ptr, ssize_t total_bytes)
{
    // number of bytes remaining to be written.
    ssize_t bytes_remaining = total_bytes;
    do {
        // attempt to write all bytes remaining.
        ssize_t bytes_written = ::write(wrfd, ptr, bytes_remaining);
        if(bytes_written == -1) {
            // error.
            return -1;
        } else {
            // update bytes_remaining.
            bytes_remaining -= bytes_written;
        }
    } while(bytes_remaining > 0);
    return total_bytes;
}

pid_t forker_t::fork()
{
    if((childpid = ::fork()) == -1) {
        std::cerr << "Unable to fork." << std::endl;
        exit(1);
    }

    if(childpid == 0) {
        // this is the child.

        // close input side of c2p.
        ::close(fdc2p[0]);
        // write fd : c2p output.
        wrfd = fdc2p[1];
    } else {
        // we are the parent.

        // close output side of c2p.
        ::close(fdc2p[1]);
        // read fd : c2p input.
        rdfd = fdc2p[0];
    }
    return childpid;
}
