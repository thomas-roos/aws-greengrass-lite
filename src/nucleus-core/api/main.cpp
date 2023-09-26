// Main blocking thread, called by containing process
#include "../lifecycle/kernel_command_line.h"
#include <c_api.h>

int ggapiMainThread(int argc, char* argv[]) {
    KernelCommandLine kernel {Global::self()};
    kernel.parseArgs(argc, argv);
    return kernel.main();
}
