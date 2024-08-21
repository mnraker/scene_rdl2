// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "TlSvr.h"
#include "LiteralUtil.h"
#include "SockUtil.h"

#include <scene_rdl2/render/util/StrUtil.h>
#include <iostream>

#include <fcntl.h>              // ::fcntl()
#include <string.h>
#ifndef _MSC_VER
    #include <netinet/in.h>         // struct sockaddr_in
    #include <netinet/tcp.h>        // TCP_NODELAY
    #include <sys/socket.h>         // ::socket()
    #include <unistd.h>             // ::read()
#else
    #include <scene_rdl2/common/platform/Endian.h> // brings in winsock2.h
    #include <ws2tcpip.h>
    #include <io.h>
#endif

namespace scene_rdl2 {
namespace grid_util {

static constexpr const char *msgHead = ">TLSvr<"; // message head strings for *MSG_CALLBACK functions

TlSvr::TlSvr() :
    mPort(-1),
    mBaseSock(-1),
    mSock(-1),
    mConnectionReady(false),
    mRecvSize(0),
    mEndLine(false)
{
}

TlSvr::~TlSvr()
{
    close();
}

int
TlSvr::open(const int serverPortNum,
            INFOMSG_CALLBACK infoMsgCallBack,
            ERRMSG_CALLBACK errMsgCallBack)
//
// You can use serverPortNum = 0 for auto search of available port by the kernel.
// In this case, you can figure out the result port number by the return value of this API.
// return opened port number, 0 is error
//
{
    mPort = serverPortNum; // You can use 0 for auto port search by kernel

    if (mPort == 0) {
        // server port open
        if (!setupServerPort(infoMsgCallBack, errMsgCallBack)) {
            mConnectionReady = false;
            return 0;           // error
        }
    }

    return mPort;
}

int
TlSvr::recv(std::string &recvStr,
            INFOMSG_CALLBACK infoMsgCallBack,
            ERRMSG_CALLBACK errMsgCallBack)
//
// return recv total byte. non blocking function.
// return size does not include last 0x0
//    0 : empty
//   -1 : EOF
//   -2 : ERROR : other error
//
{
    if (!mConnectionReady) {
        if (!setupServerPort(infoMsgCallBack, errMsgCallBack)) {
            mConnectionReady = false;
            return -2;          // error
        }

        if (mSock == -1) {
            mConnectionReady = false;
            return 0;          // still no incoming socket -> return empty status
        }

        mConnectionReady = true; // connection establised
        mRecvSize = 0;           // receive size initialize
        recvStr = "";
    }

    //
    // socket data check by select
    //
    switch (socketCheck(errMsgCallBack)) {
    case 1 :                    // active
        break;
    case 0 :                    // empty
        return 0;
    case -1 :                   // error
        return -2;              // error
    }

    //
    // read data
    //
    if (mEndLine) {
        mRecvSize = 0;          // previous session was ended by '\n'
        mEndLine = false;
        recvStr = "";
    }

    char c;
    while (1) {
#ifndef _MSC_VER
        int rSize = static_cast<int>(::read(mSock, &c, 1)); // read 1byte
#else
        int rSize = static_cast<int>(::_read(mSock, &c, 1)); // read 1byte
#endif

        if (rSize == 0) {
            if (mRecvSize == 0) {
                connectionClosed(infoMsgCallBack);
                return -1;      // EOF
            } else {
                recvStr += '\n';
                mEndLine = true;
                mRecvSize++;
            }
            
        } else if (rSize == 1) {
            if (c == '\r') {    // 0xd : CR
                continue;       // skip \r
            }

            recvStr += c;

            mRecvSize++;

            if (c == 0x0) {
                recvStr += '\n';
                mEndLine = true;
                mRecvSize++;
                break;          // end message
                
            } else if (c == '\n') {
                mEndLine = true;
                break;          // end line
            }

        } else {
            if (rSize < 0) {
                if (errno == EAGAIN) {
                    return 0;  // empty : try again

                } else if (errno == EBADF) {
                    //
                    // Probably other side of socket process is killed somehow.
                    //
                    connectionClosed(infoMsgCallBack);
                    return -1;  // EOF

                } else {
                    //
                    // error
                    //
                    if (errMsgCallBack) {
                        errMsgCallBack
                            (str_util::stringCat(msgHead, " unknown socket receive error. ",
                                                 "errno:", std::to_string(errno), " ", strerror(errno)));
                    }
                    return -2;  // error
                }
            } else {
                if (errMsgCallBack) {
                    errMsgCallBack(str_util::stringCat(msgHead, " unknown socket receive error. ",
                                                       "recvSize:", std::to_string(rSize)));
                }
                return -2;      // error
            }
        }
    }

    if (mEndLine) {
        return mRecvSize;
    }

    return 0;                  // empty or data is not terminated by '\n' yet.
}

bool
TlSvr::send(const std::string &sendStr,
            INFOMSG_CALLBACK infoMsgCallBack,
            ERRMSG_CALLBACK errMsgCallBack )
{
    if (!mConnectionReady) {
        return true;            // not ready to send -> skip
    }

    const char* cPtr = sendStr.c_str();
    int size = (int)sendStr.size();
    while (1) {
#ifndef _MSC_VER
        int wSize = static_cast<int>(::write(mSock, cPtr, size));
#else
        int wSize = static_cast<int>(::_write(mSock, cPtr, size));
#endif
        if (wSize == 0) {
            continue;           // retry 
        } else if (wSize < 0) {
            if (errno == EAGAIN) continue;       // retry
            if (errno == EINTR) continue;        // retry
            if (errno == EPIPE) { // Broken pipe
                connectionClosed(infoMsgCallBack); // Probably other side of connection may be closed.
                return false;
            } else {
                if (errMsgCallBack) {
                    errMsgCallBack
                        (str_util::stringCat(msgHead, " unknown socket send error. ",
                                             " errno:", std::to_string(errno), " ", strerror(errno)));
                }
                connectionClosed(infoMsgCallBack);
                return false;
            }
        } else {
            size -= wSize;
            if (size == 0) break; // sent all -> finish
            cPtr = (const char*)((uintptr_t)cPtr + (uintptr_t)wSize);
        }
    } // while(1)

    return true;
}

void
TlSvr::close()
{
    if (mSock != -1) {
#ifndef _MSC_VER
        ::close(mSock);
#else
        ::_close(mSock);
#endif
        mSock = -1;
    }

    if (mBaseSock != -1) {
#ifndef _MSC_VER
        ::close(mBaseSock);
#else
        ::_close(mBaseSock);
#endif
        mBaseSock = -1;
    }

    mConnectionReady = false;
}

bool
TlSvr::isConnectionEstablised() const
{
    return mConnectionReady;
}

//------------------------------------------------------------------------------

bool
TlSvr::setupServerPort(INFOMSG_CALLBACK infoMsgCallBack,
                       ERRMSG_CALLBACK errMsgCallBack)
{
    //
    // setup server socket stuff by non blocking access
    //
    if (mSock == -1) {
        if (mBaseSock == -1) {
            if (mPort == -1) {
                return true;    // skip
            }

            //
            // and socket bind and listen again
            // compute mBaseSock and mPort (if needed)
            //
            if (!socketBindAndListen(infoMsgCallBack, errMsgCallBack)) {
                return false;
            }
        }

        // compute mSock
        if (!acceptSocket(infoMsgCallBack, errMsgCallBack)) {
#ifndef _MSC_VER
            ::close(mBaseSock);
#else
            ::_close(mBaseSock);
#endif
            mBaseSock = -1;
            return false;
        }
    }

    return true;
}

bool
TlSvr::socketBindAndListen(INFOMSG_CALLBACK infoMsgCallBack,
                           ERRMSG_CALLBACK errMsgCallBack)
{
    if (mBaseSock != -1) {
        return true;
    }

#ifndef _MSC_VER
    if ((mBaseSock = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#else
    if ((mBaseSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#endif
        mBaseSock = -1;
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " ::socket() call failed for baseSock"));
        }
        return false;
    }

    //
    // Set the close-on-exec flag so that the socket will not get
    // inherited by child processes. Toshi (Aug/14/2014)
    //
#ifndef _MSC_VER
    fcntl(mBaseSock, F_SETFD, FD_CLOEXEC);
#endif

    //
    // set non blocking ::accept()
    //
#ifndef _MSC_VER
    fcntl(mBaseSock, F_SETFL, FNDELAY);
#else
    u_long iMode = 0;
    ioctlsocket(mBaseSock, FIONBIO, &iMode);
#endif

    //
    // Set up to reuse server addresses automatically and bind to the specified port.
    //
    int status = 1;
    (void)setsockopt(mBaseSock, SOL_SOCKET, SO_REUSEADDR, (char *)&status, sizeof(status));
    if (status == -1) {
#ifndef _MSC_VER
        ::close(mBaseSock);
#else
        ::_close(mBaseSock);
#endif
        mBaseSock = -1;
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " set socket option failed. (SO_REUSEADDR)"));
        }
        return false;
    }

    //
    // setup socket info
    //
    struct sockaddr_in in;
    memset(&in, sizeof(in), 0);
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = INADDR_ANY;
    in.sin_port = htons(static_cast<uint16_t>(mPort)); // put in net order

    //
    // bind the socket to the port number
    //
    if (::bind(mBaseSock, (struct sockaddr*)&in, sizeof(in)) < 0) {
#ifndef _MSC_VER
        ::close(mBaseSock);
#else
        ::_close(mBaseSock);
#endif
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead,
                                               " ::bind() socket failed. port:",
                                               std::to_string(mPort),
                                               " errno:",
                                               std::to_string(errno),
                                               " (",
                                               strerror(errno),
                                               ")"));
        }
        mBaseSock = -1;
        return false;
    }

    if (mPort == 0) {
        //
        // grab the port of the server
        //
        socklen_t inLen = sizeof(in);
        if (::getsockname(mBaseSock, (sockaddr*)&in, &inLen) != 0) {
#ifndef _MSC_VER
            ::close(mBaseSock);
#else
            ::_close(mBaseSock);
#endif
            mBaseSock = -1;
            if (errMsgCallBack) {
                errMsgCallBack(str_util::stringCat(msgHead, " ::getsockname() failed"));
            }
            return false;
        }
        mPort = ntohs(in.sin_port);

        if (infoMsgCallBack) {
            infoMsgCallBack(str_util::stringCat(msgHead, " opened server port:", std::to_string(mPort)));
        }
    }

    //
    // do listen
    //
    if (::listen(mBaseSock, 5) < 0) {
#ifndef _MSC_VER
        ::close(mBaseSock);
#else
        ::_close(mBaseSock);
#endif
        mBaseSock = -1;
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead,
                                               " ::listen() failed. baseSock:",
                                               std::to_string(mBaseSock)));
        }
        return false;
    }

    return true;
}

bool
TlSvr::acceptSocket(INFOMSG_CALLBACK infoMsgCallBack, ERRMSG_CALLBACK errMsgCallBack)
{
    if (mSock != -1) {
        return true;
    }

    struct sockaddr_in in2;
#ifndef _MSC_VER
    unsigned int addrlen = sizeof(in2);
    if ((mSock = ::accept(mBaseSock, (struct sockaddr *)&in2, &addrlen)) == -1) {
#else
    int addrlen = sizeof(in2);
    if ((mSock = accept(mBaseSock, (struct sockaddr *)&in2, &addrlen)) == INVALID_SOCKET) {
#endif
        int errNum = errno;
        if (errNum == EAGAIN) { // Resource temporarily unavailable => retry later
            return true;
        } else {
            if (errMsgCallBack) {
                errMsgCallBack
                    (str_util::stringCat(msgHead, " ::accept() returns error. ",
                                         "errno:", std::to_string(errNum), " ", strerror(errNum)));
            }

            // errNum = EINTR : interrupted by other system call -> we should try again ?
            // Please check Unix Network Programming P.67

            return false;
        }
    }

    //
    // set socket option
    //
    int optV = 1;               // true
    int optL = sizeof(int);
    if (::setsockopt(mSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optV, optL) < 0) {
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " set socket option (TCP_NODELAY) failed"));
        }
        return false;
    }
    if (::setsockopt(mSock, SOL_SOCKET, SO_KEEPALIVE, (char*)&optV, optL) < 0) {
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " set socket option (SO_KEEPALIVE) failed"));
        }
        return false;
    }
    if (!setSockBufferSize(mSock, SOL_SOCKET, 64_KiB)) {
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " setSockBufferSize failed"));
        }
        return false;
    }

    //
    // set non blocking
    //
#ifndef _MSC_VER
    if (::fcntl(mSock, F_SETFL, FNDELAY) < 0) {
#else
    u_long iMode = 0;
    if (ioctlsocket(mBaseSock, FIONBIO, &iMode) == SOCKET_ERROR) {
#endif
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " set non blocking status for newSocket failed"));
        }
        return false;
    }

    //------------------------------

    if (infoMsgCallBack) {
        infoMsgCallBack
            (str_util::stringCat(msgHead, " connection established. port:", std::to_string(mPort)));
    }

    //
    // close base socket in order to refuse other connection.
    //
#ifndef _MSC_VER
    ::close(mBaseSock);
#else
    ::_close(mBaseSock);
#endif
    mBaseSock = -1;

    return true;
}

void
TlSvr::connectionClosed(INFOMSG_CALLBACK infoMsgCallBack)
{
    ::close(mSock);
#ifndef _MSC_VER
    ::close(mSock);
#else
    ::_close(mSock);
#endif
    mSock = -1;
    mConnectionReady = false;

    if (infoMsgCallBack) {
        infoMsgCallBack(str_util::stringCat(msgHead, " connection closed at the other side. ",
                                            "port:", std::to_string(mPort)));
    }
}

int
TlSvr::socketCheck(ERRMSG_CALLBACK errMsgCallBack)
{
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(mSock, &fdset);

    struct timeval tv;
    tv.tv_sec = 0;              // no timeout. mSock is non-blocking
    tv.tv_usec = 0;

    int ret = ::select(mSock + 1, &fdset, 0x0, 0x0, &tv);
    if (ret == 0) {
        return 0;               // empty
    } else if (ret < 0) {
        if (errMsgCallBack) {
            errMsgCallBack(str_util::stringCat(msgHead, " ::select() failed. retCode=", std::to_string(ret)));
        }
        return -1;              // error
    } else {
        if (FD_ISSET(mSock, &fdset)) {
        } else {
            return 0;           // empty
        }
    }
    return 1;                   // active
}

} // namespace grid_util
} // namespace scene_rdl2
