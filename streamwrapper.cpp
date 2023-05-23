#include <arpa/inet.h>
#include "streamwrapper.h"

StreamWrapper::StreamWrapper(int sd, struct sockaddr_in* address) : m_sd(sd) {
    char ip[50];
    inet_ntop(PF_INET, (struct in_addr*)&(address->sin_addr.s_addr), ip, sizeof(ip)-1);
    m_peerIP = ip;
    m_peerPort = ntohs(address->sin_port);
}

StreamWrapper::~StreamWrapper() {
    close(m_sd);
}

ssize_t StreamWrapper::send(const char* buffer, size_t len)  {
    return write(m_sd, buffer, len);
}

ssize_t StreamWrapper::receive(char* buffer, size_t len) {
    return read(m_sd, buffer, len);
}

string StreamWrapper::getPeerIP() {
    return m_peerIP;
}

int StreamWrapper::getPeerPort() {
    return m_peerPort;
}