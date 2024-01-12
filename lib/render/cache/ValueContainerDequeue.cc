// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "ValueContainerDequeue.h"

#include <scene_rdl2/common/except/exceptions.h>

namespace scene_rdl2 {
namespace cache {

ValueContainerDequeue::ValueContainerDequeue(const void *addr, const size_t dataSize) :
    mCurrPtr(addr),
    mAddr(addr),
    mDataSize(0)
{
    dataSizeCheck(addr, dataSize);
}

ValueContainerDequeue::ValueContainerDequeue(const void *addr, const size_t dataSize, bool sizeCheck) :
    mCurrPtr(addr),
    mAddr(addr),
    mDataSize(0)
{
    if (!sizeCheck) {
        skipByteData(sizeof(size_t)); // skip enbeded dataSize data
        mDataSize = dataSize;
        return;
    }
    
    dataSizeCheck(addr, dataSize);
}

std::string
ValueContainerDequeue::show(const std::string &hd) const
{
    std::ostringstream ostr;
    ostr << hd << "ValueContainerDequeue {\n"
         << hd << "   mCurrPtr:0x" << std::hex << mCurrPtr << std::dec << '\n'
         << hd << "      mAddr:0x" << std::hex << mAddr << std::dec << '\n'
         << hd << "  mDataSize:" << mDataSize << " byte\n"
         << hd << "}";
    return ostr.str();
}

//------------------------------------------------------------------------------------------

void
ValueContainerDequeue::dataSizeCheck(const void *addr, const size_t dataSize)
{
    if (dataSize < sizeof(size_t)) {
        throw except::RuntimeError("Could not get ValueContainerDequeue header size info.");
    }

    size_t savedDataSize;
    loadSizeT(getDeqDataAddrUpdate(sizeof(size_t)), savedDataSize);
    if (savedDataSize != 0 && dataSize != savedDataSize) {
        std::ostringstream ostr;
        ostr << "Encode data length is not match with ValueContainerDequeue header. dataSize:"
             << dataSize << " != header:" << savedDataSize;
        throw except::RuntimeError(ostr.str());
    }
    mDataSize = dataSize;
}

} // namespace rdl2
} // namespace scene_rdl2

