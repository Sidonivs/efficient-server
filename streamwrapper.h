#ifndef __streamwrapper_h__
#define __streamwrapper_h__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

using namespace std;

class StreamWrapper {
    int     m_sd;
    string  m_peerIP;
    int     m_peerPort;

  public:
    friend class Acceptor;

    ~StreamWrapper();

    ssize_t send(const char* buffer, size_t len);
    ssize_t receive(char* buffer, size_t len);

    string getPeerIP();
    int    getPeerPort();

    enum {
        connectionClosed = 0,
        connectionReset = -1,
        connectionTimedOut = -2
    };

  private:
    bool waitForReadEvent(int timeout);
    
    StreamWrapper(int sd, struct sockaddr_in* address);
    StreamWrapper();
    StreamWrapper(const StreamWrapper& stream);
};

#endif