// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include "DsoFinder.h"

#include <scene_rdl2/render/util/Args.h>
#include <scene_rdl2/render/util/GetEnv.h>

#include <cstdlib>
#include <vector>

#if __cplusplus >= 201703L
    #include <filesystem>
    namespace fs = std::filesystem;
    const std::string g_raasRender("raas_render");
#else
    #include <dirent.h>
    #include <libgen.h>
#endif

namespace scene_rdl2 {
namespace rdl2 {

#ifndef _MSC_VER
const std::string os_pathsep(":");
#else
const std::string os_pathsep(";");
#endif

using util::Args;

#if __cplusplus < 201703L
int isMatching(const dirent* entry) {
    std::string name = "raas_render";
    if (name == std::string(entry->d_name)) {
        return 1;
    }
    
    return 0;
}
#endif

std::string
DsoFinder::guessDsoPath()
{
    std::string dsoPath = "";

    // First, search PATH for raas_render executable
    const std::string pathEnv = util::getenv<std::string>("PATH");
    if (pathEnv.empty()) {
        return "";
    }
#if __cplusplus >= 201703L
    size_t found = pathEnv.find(os_pathsep);
    fs::path path;
    if (found == std::string::npos) { // single path
        path = fs::path(pathEnv).make_preferred();

        if (fs::exists(path)) {
            for (auto const& dirEntry : std::filesystem::directory_iterator(path)) {
                if (dirEntry.path().filename().string() == g_raasRender) {
                    break;
                }
            }
        }
    } else {
        int counter = 0;
        bool pathFound = false;
        while (found != std::string::npos) {
            path = fs::path(pathEnv.substr(counter, found - counter)).make_preferred();
            if (fs::exists(path)) {
                for ( const auto & dirEntry : std::filesystem::directory_iterator(path)) {
                    std::string file = dirEntry.path().stem().string();
                    if (file == g_raasRender) {
                        pathFound = true;
                        break;
                    }
                }
            }
            if (pathFound) {
                break;
            }
            counter = found + 1;
            found = pathEnv.find(os_pathsep, found + 1);
        }
    }

    if (!path.empty()) {
        dsoPath = fs::path(fs::absolute(path.parent_path()) / "rdl2dso").make_preferred().string();
    }
    return dsoPath;
#else
    dirent** nameList;
    
    size_t found = pathEnv.find(os_pathsep);
    int numFound;
    std::string path;
    if (found == std::string::npos) { // single path
        path = pathEnv;
        numFound = scandir(path.c_str(), &nameList, isMatching, alphasort);
    } else {
        int counter = 0;
        while (found != std::string::npos) {
            path = pathEnv.substr(counter, found - counter);
            numFound = scandir(path.c_str(), &nameList, isMatching, alphasort);
            if (numFound > 0) {
                break;
            }
            counter = found + 1;
            found = pathEnv.find(os_pathsep, found + 1);
        }
        
        if (numFound <= 0) { // Haven't found raas_render yet
            // Process last path
            path = pathEnv.substr(counter);
            numFound = scandir(path.c_str(), &nameList, isMatching, alphasort);
        }
    }
    
    if (numFound > 0) {
        // We found raas_render, now construct path to rdl2dso
        // This assumes that the immediate parent directory is /bin
        char* buf = realpath(path.c_str(), NULL); // Resolve any relative links
        dsoPath = std::string(dirname(buf)) + "/" + "rdl2dso";
        free(buf);
    }
    
    // clean up
    /*while (numFound--) {
        free(nameList[numFound]);
    }*/
    free(nameList);
    return dsoPath;
#endif
}

std::string DsoFinder::find() {
    // check if RDL2_DSO_PATH is set
    std::string dsoPathString = "."; // Search '.' path first
    if (const char* const dsoPathEnvVar = util::getenv<const char*>("RDL2_DSO_PATH")) {
        // append dso path as sourced from RDL2_DSO_PATH
        dsoPathString += os_pathsep + std::string(dsoPathEnvVar);
    }
    
    // finally, guess dso path based on location of raas_render
    std::string guessedDsoPath = guessDsoPath();
    if (!guessedDsoPath.empty()) {
        // append dso path as sourced from location of raas_render executable
        dsoPathString += os_pathsep + guessedDsoPath;   
    }
    
    return dsoPathString;
}

std::string DsoFinder::parseDsoPath(int argc, char* argv[]) {
    Args args(argc, argv);
    Args::StringArray values;
    std::string dsoPath;
    
    int foundAtIndex = args.getFlagValues("--dso_path", 1, values);
    while (foundAtIndex >= 0) {
        dsoPath = values[0];
        foundAtIndex = args.getFlagValues("--dso_path", 1, values, foundAtIndex + 1);
    }
    
    foundAtIndex = args.getFlagValues("--dso-path", 1, values);
    while (foundAtIndex >= 0) {
        dsoPath = values[0];
        foundAtIndex = args.getFlagValues("--dso-path", 1, values, foundAtIndex + 1);
    }

    foundAtIndex = args.getFlagValues("-d", 1, values);
    while (foundAtIndex >= 0) {
        dsoPath = values[0];
        foundAtIndex = args.getFlagValues("-d", 1, values, foundAtIndex + 1);
    }
    
    std::string findPath = find();
    
    if (!dsoPath.empty()) {
        // prepend dso path as sourced from command line
        return dsoPath + os_pathsep + findPath; 
    }
    
    return findPath; 
}

}
}

