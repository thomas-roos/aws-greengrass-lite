// Main blocking thread, called by containing process
#include "../lifecycle/kernel_command_line.h"
#include <c_api.h>

int ggapiMainThread(int argc, char* argv[], char* envp[]) {
    Global & global = Global::self();
    if (envp != nullptr) {
        global.environment.sysProperties.parseEnv(envp);
    }
    KernelCommandLine kernel {global};
    kernel.parseEnv(global.environment.sysProperties);
    if (argc > 0 && argv != nullptr) {
        kernel.parseArgs(argc, argv);
    }
    return kernel.main();
}
