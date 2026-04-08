#include "kys_main.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    return Run(argc, argv);
}