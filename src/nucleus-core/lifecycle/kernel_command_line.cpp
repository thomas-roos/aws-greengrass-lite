#include "kernel_command_line.h"
#include "../data/shared_struct.h"
#include "../data/globals.h"
#include "c_api.h"
#include <optional>

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
    if (progName.empty()) {
        return;
    }
    std::filesystem::path progPath {progName};
    progPath = absolute(progPath);
    if (!exists(progPath)) {
        // assume this is not usable for extracting directory information
        return;
    }
    std::filesystem::path parent { progPath.parent_path() };
    if (parent.filename().generic_string() == "bin") {
        parent = parent.parent_path(); // strip the /bin
    }
    // TODO:
    /* rootPath = parent; */
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
    std::string rootAbsolutePath; // TODO: see parse filename
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

int KernelCommandLine::main() {
    Global & global = Global::self();
    auto threadTask = ggapiClaimThread(); // assume long-running thread, this provides a long-running task handle

    // This needs to be subsumed by the lifecyle management - not yet implemented, so current approach is hacky
    global.loader->discoverPlugins();
    std::shared_ptr<Structish> emptyStruct {std::make_shared<SharedStruct>(global.environment)}; // TODO, empty for now
    global.loader->lifecycleBootstrap(emptyStruct);
    global.loader->lifecycleDiscover(emptyStruct);
    global.loader->lifecycleStart(emptyStruct);
    global.loader->lifecycleRun(emptyStruct);

    (void)ggapiWaitForTaskCompleted(threadTask, -1); // essentially blocks forever but allows main thread to do work
    global.loader->lifecycleTerminate(emptyStruct);
    return 0; // currently unreachable
}
