#pragma once
#include "globals.h"
#include <optional>

class KernelCommandLine {
private:
    Global & _global;
    std::string _providedConfigPathName;
    std::string _providedInitialConfigPath;
    std::string _awsRegionFromCmdLine;
    std::string _envStageFromCmdLine;
    std::string _defaultUserFromCmdLine;

    std::string _configPathName { "~root/config" };
    std::string _workPathName { "~root/work" };
    std::string _packageStorePathName { "~root/packages" };
    std::string _kernelAltsPathName { "~root/alts" };
    std::string _deploymentsPathName { "~root/deployments" };
    std::string _cliIpcInfoPathName { "~root/cli_ipc_info" };
    std::string _binPathName { "~root/bin" };

    static std::string nextArg(const std::vector<std::string> & args, std::vector<std::string>::const_iterator & iter);
    std::string deTilde(std::string_view s);

public:
    KernelCommandLine(Global & global) : _global{global} {
    }
    static int main(int argc, char * argv[]);
    void parseArgs(int argc, char * argv[]);
    void parseArgs(const std::vector<std::string> & args);
    void parseProgramName(std::string_view progName);
};
