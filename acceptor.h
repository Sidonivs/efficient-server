#include <string>
#include <netinet/in.h>
#include "streamwrapper.h"

using namespace std;

class Acceptor
{
    int    m_lsd;
    string m_address;
    int    m_port;
    bool   m_listening;

  public:
    Acceptor(int port, const char* address="");
    ~Acceptor();

    int       start();
    StreamWrapper* accept();

  private:
    Acceptor() {}
};