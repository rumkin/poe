#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <map>
#include <queue>
#include <functional>
#include "uv.h"


namespace Poe {
    class Server;
    class Connection;
    class Request;
    class Response;
    class Transport;
    // Stream encoder/decoder
    class Codec;
    class Pack;
    
    typedef std::map<int, Connection *> Pool;
    typedef std::vector<char> Buffer;
    typedef std::queue<Request *> Requests;
    typedef std::function<void(Request *, Response *)> RequestCallback;
    typedef std::function<void(Server *, Connection *)> ConnectionCallback;
    
    class Server {
        Pool * pool;
        Requests * requests;
        Codec * codec;
        Transport * transport;
        // Internal ID counter
        int counter = 0;
        // Maximum connections
        int max_connections = 125;
        bool is_started = false;
        RequestCallback on_request = 0;
        
        ConnectionCallback on_connect = 0;
        ConnectionCallback on_disconnect = 0;
    public:
        Server();
        Server(RequestCallback);
        Server(Codec *, RequestCallback);
        ~Server();
        // Transport
        Transport * GetTransport();
        // Connections
        void MaxConns(int);
        bool HasConn(int i);
        Connection * FindConn(int i);
        Connection * GetConn(int i);
        Connection * OpenConn();
        void CloseConn(Connection *);
        void CloseConn(int);
        int CountConns();
        
        // Connection Events
        void OnConnect(ConnectionCallback);
        void OnDisconnect(ConnectionCallback);
        void OnRequest(RequestCallback);
        
        // Requests
        void AddRequest(Request *);
        bool HasRequest();
        Request * GetRequest();
        int CountRequests();
        // Response
        void SendResponse(Response *);
        
        // Flow
        bool IsStarted();
        // Start on 127.0.0.1 at any port
        int Start();
        // Start on 0.0.0.0 with port nu,ber
        int Start(int);
        // Start on special ip and port
        int Start(const char *, int);
        // Start on unix path (currently unavailable)
        // void Start(const char *);
        void Stop();
        bool IsRunning();
    };
    
    class TransportConn {
        public:
        Transport * transport;
        Connection * conn;
        uv_tcp_t * stream;
    };
    
    class Transport {
        Server * server;
        uv_tcp_t * tcp;
        bool is_started = false;
        public:
        ~Transport();
        
        std::map<int, TransportConn *> connections = {};
        uv_loop_t * loop;
        
        void SetServer(Server * server);
        Server * GetServer();
        void Write(Connection *, const char*, size_t);
        void Write(int, const char*, size_t);
        bool Loop();
        int Start();
        int Start(int);
        int Start(const char*, int);
        void Stop();
        void CloseConn(int id);
        bool HasConn(int id);
        TransportConn * OpenConn();        
    };
       
    class Codec {
        public:
        void Decode(Connection *);
        void Encode(Connection *, void *);
        void Clear(void *);
    };
    
    class Pack {
        public:
        char * data;
        size_t size;
        Pack();
        Pack(const char*, size_t);
        ~Pack();
    };
    
    class Request {
        public:
        int id = 0;
        void * data;
        Connection * conn;
        ~Request();
    };
    
    class Response {
        public:
        Connection * conn;
        void * data;
        Response(Connection *);
        Response(Request *);
        ~Response();
        void Send();
        void Send(std::function<void(Response * res)>);
    };
    
    // TODO Get connection remote address
    class Connection {
        bool is_closed = false;
        Codec * codec;
        
    public:
        Server * server;
        int id;
        int counter = 0;
        Buffer buffer = Buffer(); // TODO Move buffer to Codec?
        
        Connection();
        ~Connection();
        
        bool IsClosed();
        void Close();
        void * data;
        // Push incoming data
        void Push(char *, size_t);
        // Write encoded data
        void Write(void*);
        // Write pure data
        void Write(const char *, size_t);
        
        void SetCodec(Codec *);
        Codec * GetCodec();
    };
}

#endif