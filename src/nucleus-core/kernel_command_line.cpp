#include "kernel_command_line.h"
#include "shared_struct.h"
#include "globals.h"
#include <cpp_api.h>
#include <optional>

// Main blocking thread, called by containing process

int ggapiMainThread(int argc, char* argv[]) {
    KernelCommandLine kernel {Global::self()};
    kernel.parseArgs(argc, argv);
    return kernel.main(argc, argv);
    Global & global = Global::self();
    auto threadTask = ggapi::ObjHandle::claimThread(); // assume long-running thread, this provides a long-running task handle

    // This needs to be subsumed by the lifecyle management - not yet implemented, so current approach is hacky
    global.loader->discoverPlugins();
    std::shared_ptr<Structish> emptyStruct {std::make_shared<SharedStruct>(global.environment)}; // TODO, empty for now
    global.loader->lifecycleBootstrap(emptyStruct);
    global.loader->lifecycleDiscover(emptyStruct);
    global.loader->lifecycleStart(emptyStruct);
    global.loader->lifecycleRun(emptyStruct);

    (void)threadTask.waitForTaskCompleted(); // essentially blocks forever but allows main thread to do work
    // TODO: This is currently crashing!
    global.loader->lifecycleTerminate(emptyStruct);
    return 0; // currently unreachable
}

void KernelCommandLine::parseArgs(int argc, char * argv[]) {
    std::vector<std::string> args;
    args.reserve(argc-1);
    if (argv[0]) {
        parseProgramName(argv[0]);
    } else {
        throw std::runtime_error("Handle nullptr");
    }
    for (int i = 1; i < argc; i++) {
        args.emplace_back(argv[i]);
    }
    parseArgs(args);
}

void KernelCommandLine::parseProgramName(std::string_view progName) {

}

std::string KernelCommandLine::nextArg(const std::vector<std::string> & args, std::vector<std::string>::const_iterator & iter) {
    if (iter == args.end()) {
        throw std::runtime_error("Expecting argument");
    }
    std::string v = *iter;
    ++iter;
    return v;
}

void KernelCommandLine::parseArgs(const std::vector<std::string> & args) {
    std::string rootAbsolutePath;
    for (auto i = args.begin(); i != args.end(); ) {
        std::string op = nextArg(args, i);
        // TODO: GG-Java ignores case
        if (op == "--config" || op == "-i") {
            std::string arg = nextArg(args, i);
            _providedConfigPathName = deTilde(arg);
            continue;
        }
        if (op == "--init-config" || op == "-init") {
            std::string arg = nextArg(args, i);
            _providedInitialConfigPath = deTilde(arg);
            continue;
        }
        if (op == "--root" || op == "-r") {
            std::string arg = nextArg(args, i);
            rootAbsolutePath = deTilde(arg);
            continue;
        }
        if (op == "--aws-region" || op == "-ar") {
            std::string arg = nextArg(args, i);
            _awsRegionFromCmdLine = arg;
            continue;
        }
        if (op == "--env-stage" || op == "-es") {
            std::string arg = nextArg(args, i);
            _envStageFromCmdLine = arg;
            continue;
        }
        if (op == "--component-default-user" || op == "-u") {
            std::string arg = nextArg(args, i);
            _defaultUserFromCmdLine = arg;
            continue;
        }
        throw std::runtime_error(std::string("Unrecognized command: ") + op);
    }
    if (rootAbsolutePath.empty() && !_providedInitialConfigPath.empty() &&
                exists(std::filesystem::path(_providedInitialConfigPath))) {
        throw std::runtime_error("TODO: read path from config");
    }
    if (rootAbsolutePath.empty()) {
        rootAbsolutePath = "~/.greengrass";
    }
    rootAbsolutePath = deTilde(rootAbsolutePath);
}

int KernelCommandLine::main(int argc, char * argv[]) {

}