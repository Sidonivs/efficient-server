#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>

#include "threadwrapper.h"
#include "workqueue.h"
#include "acceptor.h"
#include "streamwrapper.h"
#include "graph.h"

#include "gen/schema.pb.h"

class WorkItem {
    StreamWrapper* m_stream;
 
  public:
    WorkItem(StreamWrapper* stream) : m_stream(stream) {}
    ~WorkItem() { delete m_stream; }
 
    StreamWrapper* getStream() { return m_stream; }
};

class ConnectionHandler : public ThreadWrapper {
    WorkQueue<WorkItem*>& m_queue;
    Graph& m_graph;
    vector<esw::server::Walk> walks_buffer;
 
  public:
    ConnectionHandler(WorkQueue<WorkItem*>& queue, Graph& graph) : m_queue(queue), m_graph(graph) {}
 
    void* run() {
        for (int loop = 0;; loop++) {
            cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - waiting..." << endl;
            WorkItem* item = m_queue.remove();
            StreamWrapper* stream = item->getStream();

            char msg_len_input[4];
            int len;
            int proto_len;
            while ((len = stream->receive(msg_len_input, 4)) > 0 ){
                uint32_t msg_len = *((uint32_t*) msg_len_input);
                msg_len = __bswap_32(msg_len);

                //cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - msg len: " << msg_len << endl;
                
                char input[msg_len];
                proto_len = 0;
                while (proto_len != msg_len) {
                    char buf[msg_len-proto_len];
                    int new_len = stream->receive(buf, msg_len - proto_len);
                    memcpy(input + proto_len, buf, sizeof(buf));
                    proto_len += new_len;
                }

                esw::server::Request request;
                if(!request.ParseFromArray(input, msg_len)) {
                    cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - ERROR PARSING MESSAGE" << endl;
                }

                if (request.has_walk()) {
                    //printf("thread %lu, loop %d - Walk\n", 
                    //    (long unsigned int)self(), loop);

                    walks_buffer.push_back(request.walk());
                    /*
                    for (int i = 0; i < request.walk().locations_size(); ++i) {
                        cout << "X " << request.walk().locations(i).x() << " Y " << request.walk().locations(i).y() << endl;
                    }
                    for (int i = 0; i < request.walk().lengths_size(); ++i) {
                        cout << request.walk().lengths(i) << " ";
                    }
                    */
                    // respond
                    esw::server::Response response;
                    response.set_status(esw::server::Response_Status_OK);
                    uint32_t send_len = (uint32_t)response.ByteSize();
                    char send_buffer[send_len];
                    response.SerializeToArray(send_buffer, send_len);
                    char len_buf[4];
                    // convert the unsigned integer from host byte order to network byte order
                    uint32_t nt_byte_order_len = htonl(send_len);
                    // create buffer in which to send message length
                    len_buf[0] = nt_byte_order_len;
                    len_buf[1] = nt_byte_order_len >> 8;
                    len_buf[2] = nt_byte_order_len >> 16;
                    len_buf[3] = nt_byte_order_len >> 24;
                    // send message length
                    stream->send(len_buf, 4);
                    //send message
                    stream->send(send_buffer, send_len);

                } else if (request.has_onetoone()) {
                    printf("thread %lu, loop %d - OneToOne\n", 
                        (long unsigned int)self(), loop);

                    //cout << "ORIGIN: X " << request.onetoone().origin().x() << " Y " << request.onetoone().origin().y() << endl;

                    m_graph.add_walks(walks_buffer);
                    cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - walks added." << endl;
                    walks_buffer.clear();
                    Node origin = {request.onetoone().origin().x(), request.onetoone().origin().y()};
                    Node destination = {request.onetoone().destination().x(), request.onetoone().destination().y()};
                    uint64_t shortest_path_length = m_graph.find_shortest_path_length(origin, destination);
                    cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - shortest path length: " << shortest_path_length << endl;

                    // respond
                    esw::server::Response response;
                    response.set_status(esw::server::Response_Status_OK);
                    response.set_shortest_path_length(shortest_path_length);
                    uint32_t send_len = (uint32_t)response.ByteSize();
                    char send_buffer[send_len];
                    response.SerializeToArray(send_buffer, send_len);
                    char len_buf[4];
                    // convert the unsigned integer from host byte order to network byte order
                    uint32_t nt_byte_order_len = htonl(send_len);
                    // create buffer in which to send message length
                    len_buf[0] = nt_byte_order_len;
                    len_buf[1] = nt_byte_order_len >> 8;
                    len_buf[2] = nt_byte_order_len >> 16;
                    len_buf[3] = nt_byte_order_len >> 24;
                    // send message length
                    stream->send(len_buf, 4);
                    //send message
                    stream->send(send_buffer, send_len);

                } else if (request.has_onetoall()) {
                    printf("thread %lu, loop %d - OneToAll\n", 
                        (long unsigned int)self(), loop);

                    m_graph.add_walks(walks_buffer);
                    walks_buffer.clear();
                    Node origin = {request.onetoall().origin().x(), request.onetoall().origin().y()};
                    uint64_t total_length = m_graph.find_total_length(origin);
                    cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - total length: " << total_length << endl;

                    // respond
                    esw::server::Response response;
                    response.set_status(esw::server::Response_Status_OK);
                    response.set_total_length(total_length);
                    uint32_t send_len = (uint32_t)response.ByteSize();
                    char send_buffer[send_len];
                    response.SerializeToArray(send_buffer, send_len);
                    char len_buf[4];
                    // convert the unsigned integer from host byte order to network byte order
                    uint32_t nt_byte_order_len = htonl(send_len);
                    // create buffer in which to send message length
                    len_buf[0] = nt_byte_order_len;
                    len_buf[1] = nt_byte_order_len >> 8;
                    len_buf[2] = nt_byte_order_len >> 16;
                    len_buf[3] = nt_byte_order_len >> 24;
                    // send message length
                    stream->send(len_buf, 4);
                    //send message
                    stream->send(send_buffer, send_len);

                } else if (request.has_reset()) {
                    //printf("thread %lu, loop %d - Reset\n", 
                    //    (long unsigned int)self(), loop);

                    m_graph.reset();

                    // respond
                    esw::server::Response response;
                    response.set_status(esw::server::Response_Status_OK);
                    uint32_t send_len = (uint32_t)response.ByteSize();
                    char send_buffer[send_len];
                    response.SerializeToArray(send_buffer, send_len);
                    char len_buf[4];
                    // convert the unsigned integer from host byte order to network byte order
                    uint32_t nt_byte_order_len = htonl(send_len);
                    // create buffer in which to send message length
                    len_buf[0] = nt_byte_order_len;
                    len_buf[1] = nt_byte_order_len >> 8;
                    len_buf[2] = nt_byte_order_len >> 16;
                    len_buf[3] = nt_byte_order_len >> 24;
                    // send message length
                    stream->send(len_buf, 4);
                    //send message
                    stream->send(send_buffer, send_len);

                } else {
                    printf("thread %lu, loop %d - ERROR REQUEST HAS NO MESSAGE\n", 
                        (long unsigned int)self(), loop);
                }
            }

            //cout << "thread " << (long unsigned int)self() << ", loop " <<  loop << " - ENDIIIING" << endl;
            
            delete item;
        }

        return NULL;
    }
};

int main(int argc, char** argv) {
    if ( argc < 3 || argc > 4 ) {
        printf("usage: %s <workers> <port> <ip>\n", argv[0]);
        exit(-1);
    }
    int workers = atoi(argv[1]);
    int port = atoi(argv[2]);
    string ip;
    if (argc == 4) { 
        ip = argv[3];
    }
 
    WorkQueue<WorkItem*> queue;
    Graph graph;
    for (int i = 0; i < workers; i++) {
        ConnectionHandler* handler = new ConnectionHandler(queue, graph);
        if (!handler) {
            printf("Could not create ConnectionHandler %d\n", i);
            exit(1);
        } 
        handler->start();
    }
 
    // Create an acceptor then start listening for connections
    WorkItem* item;
    Acceptor* connectionAcceptor;
    if (ip.length() > 0) {
        connectionAcceptor = new Acceptor(port, (char*)ip.c_str());
    }
    else {
        connectionAcceptor = new Acceptor(port);        
    }                                        
    if (!connectionAcceptor || connectionAcceptor->start() != 0) {
        printf("Could not create connection acceptor\n");
        exit(1);
    }

    // Add a work item to the queue for each connection
    while (1) {
        StreamWrapper* connection = connectionAcceptor->accept();
        if (!connection) {
            printf("Could not accept connection\n");
            continue;
        }
        item = new WorkItem(connection);
        if (!item) {
            printf("Could not create work item from connection\n");
            continue;
        }
        queue.add(item);
    }
 
    exit(0);
}