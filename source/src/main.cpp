#include <string.h>
#include <stdio.h>
#include "sld.h"
#include "sle.h"
#include "lcmp.h"
#include "lcheck.h"
#include "lle.h"
#include "simplify.h"


int main(int argc, char* argv[])
{
    char* progname = new char[strlen(argv[0])+1];
    strcpy(progname, argv[0]);
    char* baseprogname = basename(progname);
    std::string baseprog(baseprogname);

    delete [] progname;
    progname = NULL;
    baseprogname = NULL;

    if(baseprog == "sld") {
        return sld_main(argc, argv);
    } else if(baseprog == "sle") {
        return sle_main(argc, argv);
    } else if(baseprog == "lle") {
        return lle_main(argc, argv);
    } else if(baseprog == "lcmp") {
        return lcmp_main(argc, argv);
    } else if(baseprog == "lcheck") {
        return lcheck_main(argc, argv);
    } else if(baseprog == "simplify") {
        return simplify_main(argc, argv);
    } else {
        fprintf(stderr, "Unknown invocation: %s\n", baseprogname);
        return 1;
    }
}
