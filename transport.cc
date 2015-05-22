#include <iostream>
#include <map>
#include "poe.h"
#include "uv.h"
#include <functional>

namespace Poe {
    void alloc_buffer(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
        buf->base = (char*) malloc(sizeof(char) * size);
        buf->len = size;
    }
    
    void on_write(uv_write_t* req, int status) {
        if (status < 0) {
            fprintf(stderr, "Write error %s\n", uv_strerror(status));
            return;
        }
    
        uv_buf_t * buf = (uv_buf_t*) req->data;
        free(buf->base);
        free(buf);
        free(req);
    }
    
    void on_close(uv_handle_t* handle) {
        TransportConn * conn = reinterpret_cast<TransportConn *>(handle->data);
        delete conn;
        
        free(handle);
    }
    
    void on_close_empty(uv_handle_t* handle) {
        free(handle);
    }

    // Callbacks
    void on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
        TransportConn * conn = reinterpret_cast<TransportConn *>(handle->data);
        
        if (nread == -4095) {
            conn->transport->CloseConn(conn->conn->id);
            free(buf->base);
            return;
        }
    
        if (nread < 0) {
            fprintf(stderr, "%s\n", uv_strerror(nread));
            uv_close((uv_handle_t*) handle, on_close);
            free(buf->base);
            return;
        }
    
        conn->conn->Push(buf->base, nread);
    
        free(buf->base);
    }

    void on_new_connection(uv_stream_t *tcp, int status) {
        Transport * transport = reinterpret_cast<Transport *>(tcp->data);
        
        if (status < 0) {
            fprintf(stderr, "New connection error %s\n", uv_strerror(status));
            // error!
            transport->GetServer()->Stop();
            return;
        }
    
        uv_tcp_t * handle = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
        uv_tcp_init(transport->loop, handle);
        
        if (uv_accept(tcp, (uv_stream_t*) handle) == 0) {
            TransportConn * conn = transport->OpenConn();
            conn->stream = handle;

            handle->data = (void*) conn;
            
            uv_read_start((uv_stream_t*) handle, alloc_buffer, on_read);
        } else {
            uv_close((uv_handle_t*) handle, on_close_empty);
        }
    }
    
   // Transport  ---------------------------------------------------------------
   int backlog = 125;
   
   Transport::~Transport() {
       connections.clear();
   }
   
   void Transport::SetServer(Server * server) {
      this->server = server;
   }
   
   Server * Transport::GetServer() {
      return server;
   }
   
   int Transport::Start() {
       return Start(0);
   }
   
   int Transport::Start(int port) {
       return Start("0.0.0.0", port);
   }
   
    int Transport::Start(const char* ip, int port) {
        if (is_started) {
            fprintf(stderr, "Already started\n");
            return 0;
        }
        
        loop = uv_default_loop();
        tcp = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
        
        uv_tcp_init(loop, tcp);
        struct sockaddr_in addr;
    
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_bind(tcp, reinterpret_cast<const sockaddr*>(&addr), 0);
    
        tcp->data = (void *) this;
    
        int r = uv_listen((uv_stream_t*) tcp, backlog, on_new_connection);
    
        if (r) {
            fprintf(stderr, "Listen error %s\n", uv_strerror(r));
            return 1;
        }
        
        is_started = true;
        return 0;
    }
   
   bool Transport::Loop() {
       if (! is_started) return false;
       
       int result = uv_run(loop, UV_RUN_NOWAIT);
       
       return result > 0;
   }
   
    void Transport::Stop() {
        if (! is_started) return;
       
        TransportConn * conn;
        std::map<int, TransportConn *>::iterator it;
        while (! connections.empty()) {
            it = connections.begin();
            conn = it->second;
            uv_close((uv_handle_t*) conn->stream, on_close);
            // delete conn;
            connections.erase(it);
        }
        
        int e;
        e = uv_run(loop, UV_RUN_ONCE);
        
        uv_stop(loop);
        
        e = uv_loop_close(loop);
        if (e != 0 && e != -16) {
            fprintf(stderr, "Error #%i: %s\n", -e, uv_strerror(e));
        }
        
        free(tcp);
        is_started = false;
    }
    
    TransportConn * Transport::OpenConn() {
        TransportConn * conn = new TransportConn();
        
        conn->transport = this;
        conn->conn = this->GetServer()->OpenConn();
        
        connections.insert(std::pair<int, TransportConn*>(conn->conn->id, conn));
        
        return conn;
    }
    
    void Transport::CloseConn(int id) {
        std::map<int, TransportConn*>::iterator it = connections.find(id);
        if (it == connections.end()) return;
        
        TransportConn * conn = it->second;
        
        connections.erase(id);
        uv_close((uv_handle_t*) conn->stream, on_close);
        server->CloseConn(conn->conn);
    }
    
    bool Transport::HasConn(int id) {
        return connections.find(id) != connections.end();
    }
    
   
   void Transport::Write(int id, const char * data, size_t size) {
        TransportConn * conn = connections.find(id)->second;
        
        uv_buf_t buf = {};
        
        buf.base = (char *) data;
        buf.len = size;
        
        uv_buf_t response[] = {
            buf
        };
        
        uv_write_t *res = (uv_write_t *) malloc(sizeof(uv_write_t));
        uv_write(res, (uv_stream_t *) conn->stream, response, 1, [](uv_write_t * res, int status){
            if (status != 0) {
                fprintf(stderr, "Write error %s\n", uv_strerror(status));
            }
            
            free(res);
        });
   }
}