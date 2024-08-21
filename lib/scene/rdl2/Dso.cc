// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0


#include "Dso.h"

#include "SceneClass.h"
#include "SceneObject.h"
#include "Types.h"

#include <scene_rdl2/common/platform/Platform.h>
#include <scene_rdl2/common/except/exceptions.h>
#include <scene_rdl2/render/util/Files.h>
#include <scene_rdl2/render/util/Strings.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>
#include <string>

#ifndef _MSC_VER
    #include <dlfcn.h>
    #include <unistd.h>
#endif

#include <filesystem>

namespace {
#ifdef __WIN32__

typedef HMODULE DynamicLibrary;

__forceinline std::string __dlerror()
{
    char *pMsgBuf;
    DWORD error = GetLastError();
    if(error != NO_ERROR)
    {
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&pMsgBuf),
            0, NULL);
        std::string errorStr = pMsgBuf;
        LocalFree(pMsgBuf);
        return std::move(errorStr);
    }
    return std::string();
}

#define dynamic_library_open(path, rtld) LoadLibraryA(path)
#define dynamic_library_close(lib) FreeLibrary(lib)
#define dynamic_library_find(lib, symbol) GetProcAddress(lib, symbol)
#define dynamic_library_error() __dlerror()
#define dynamic_library_clearerror() SetLastError(NO_ERROR)

#else

#include <dlfcn.h>

__forceinline std::string __dlerror()
{
    const char *error = dlerror();
    if(error)
    {
        return std::move(std::string(error));
    }
    return std::string();
}

typedef void *DynamicLibrary;

#define dynamic_library_open(path, rtld) dlopen(path, rtld)
#define dynamic_library_close(lib) dlclose(lib)
#define dynamic_library_find(lib, symbol) dlsym(lib, symbol)
#define dynamic_library_error() __dlerror()
#define dynamic_library_clearerror() dlerror()

#endif
}

namespace scene_rdl2 {
namespace rdl2 {

namespace internal {

std::string
classNameFromFileName(const std::string& baseName,
                      const std::string& expectedExtension)
{
    // Base name must be at least expectedExtension + 1 chars ("a<extension>").
    if (baseName.size() < expectedExtension.size() + 1) {
        return "";
    }

    // Final characters must match expectedExtension.
    std::string extension =
        baseName.substr(baseName.size() - expectedExtension.size(), std::string::npos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    if (extension != expectedExtension) {
        return "";
    }

    // Class name is the file name without the extension.
    return baseName.substr(0, baseName.size() - expectedExtension.size());
}

} // namespace internal

/* static */
std::string
Dso::classNameFromFileName(const std::string& filePath)
{
    std::filesystem::path p(filePath);
    std::string directory(p.parent_path().string());
    std::string baseName(p.stem().string());

    // Bail early if we can't determine the class name.

    std::string className(internal::classNameFromFileName(baseName, ".so.proxy"));
    if (className.empty()) {
        className = internal::classNameFromFileName(baseName, ".so");
    }
    return className;
}


Dso::Dso(const std::string& className, const std::string& searchPath, bool proxyModeEnabled) :
    mHandle(nullptr),
    mDeclareFunc(nullptr),
    mCreateFunc(nullptr),
    mDestroyFunc(nullptr)
{
    MNRY_ASSERT(!className.empty(), "Dso must be constructed with a non-empty SceneClass name.");
    mFilePath = className + ".so";

    if (proxyModeEnabled) {
        mFilePath += ".proxy";
    }

    // If they explicitly specified a search path, attempt to find the DSO.
    if (!searchPath.empty()) {
        mFilePath = util::findFile(mFilePath, searchPath);
    }

    // If the file path is empty, we couldn't find it.
    if (mFilePath.empty()) {
        throw except::IoError(util::buildString("Couldn't find DSO for '",
                className, "' in search path '", searchPath, "'."));
    }

    // Attempt to open the DSO.
    mHandle = (void*)dynamic_library_open(mFilePath.c_str(), RTLD_LAZY);
    if (!mHandle) {
        std::stringstream errMsg;
        errMsg << "Found RDL2 DSO '" << mFilePath << "', but failed to"
            " dlopen() it";
        std::string error = dynamic_library_error();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }
}

Dso::~Dso()
{
    if (mHandle) {
        dynamic_library_close((DynamicLibrary)mHandle);
    }
}

ClassDeclareFunc
Dso::getDeclare()
{
    // Return cached function pointer if we already looked it up.
    if (mDeclareFunc) {
        return mDeclareFunc;
    }

    // Clear errors.
    dynamic_library_clearerror();

    // Attempt to load the rdl_declare symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* declareSymbol = dynamic_library_find((DynamicLibrary)mHandle, "rdl2_declare");
    if (!declareSymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_declare' from RDL2 DSO '" <<
            mFilePath << "'";
        std::string error = dynamic_library_error();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mDeclareFunc = (ClassDeclareFunc)declareSymbol;

    return mDeclareFunc;
}

ObjectCreateFunc
Dso::getCreate()
{
    // Return cached function pointer if we already looked it up.
    if (mCreateFunc) {
        return mCreateFunc;
    }

    // Clear errors.
    dynamic_library_clearerror();

    // Attempt to load the rdl_create symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* createSymbol = dynamic_library_find((DynamicLibrary)mHandle, "rdl2_create");
    if (!createSymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_create' from RDL2 DSO '" <<
            mFilePath << "'";
        std::string error = dynamic_library_error();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mCreateFunc = (ObjectCreateFunc)createSymbol;

    return mCreateFunc;
}

ObjectDestroyFunc
Dso::getDestroy()
{
    // Return the cached function pointer if we already looked it up.
    if (mDestroyFunc) {
        return mDestroyFunc;
    }

    // Clear errors.
    dynamic_library_clearerror();

    // Attempt to load the rdl_destroy symbol.
    MNRY_ASSERT(mHandle, "Tried to load symbol from bad DSO handle.");
    void* destroySymbol = dynamic_library_find((DynamicLibrary)mHandle, "rdl2_destroy");
    if (!destroySymbol) {
        // Technically null symbols are valid, but a null function pointer
        // is useless to us, so that's really a failure case, too.
        std::stringstream errMsg;
        errMsg << "Failed to load symbol 'rdl2_destroy' from RDL2 DSO '" <<
            mFilePath << "'";
        std::string error = dynamic_library_error();
        if (!error.empty()) {
            errMsg << ": " << error;
        } else {
            errMsg << ".";
        }
        throw except::RuntimeError(errMsg.str());
    }

    // Cache the function pointer.
    mDestroyFunc = (ObjectDestroyFunc)destroySymbol;

    return mDestroyFunc;
}

bool
Dso::isValidDso(const std::string& filePath, bool proxyModeEnabled)
{
    // Break the path into directory and basename components.
    std::filesystem::path p = filePath;
    std::string directory(p.parent_path().string());
    std::string baseName(p.stem().string());

    // Bail early if we can't determine the class name.
    const char* extension = (proxyModeEnabled) ? ".so.proxy" : ".so";
    std::string className = internal::classNameFromFileName(baseName, extension);
    if (className.empty()) {
        return false;
    }

    // Attempt to load it as an RDL Dso object and get the expected function
    // pointers.
    try {
        std::unique_ptr<Dso> dso(new Dso(className, directory, proxyModeEnabled));
        dso->getDeclare();
        if (!proxyModeEnabled) {
            dso->getCreate();
            dso->getDestroy();
        }
    } catch (...) {
        return false;
    }

    // It's a valid DSO.
    return true;
}

} // namespace rdl2
} // namespace scene_rdl2

