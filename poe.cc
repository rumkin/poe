#include <iostream>
#include <assert.h>
#include <cstring>
#include <algorithm>
#include <iterator>
#include "poe.h"

#include <string>
// #include <stdint.h>
#include <vector>



namespace Poe {
   
   
   
   // Server -------------------------------------------------------------------
   
   Server::Server() {
      pool = new Pool();
      requests = new Requests();
      codec = new Codec(); 
      transport = new Transport();
      transport->SetServer(this);
   }
   
   Server::Server(RequestCallback on_request) {
      this->on_request = on_request;
      pool = new Pool();
      requests = new Requests();
      codec = new Codec();
      transport = new Transport();
      transport->SetServer(this);
   }
   
   Server::Server(Codec * codec, RequestCallback on_request) {
      this->on_request = on_request;
      this->codec = codec;
      
      pool = new Pool();
      requests = new Requests();
      transport = new Transport();
      transport->SetServer(this);
   }
   
   Server::~Server() {
      Pool::iterator it;
      Connection * conn;
      
      while (! pool->empty()) {
         it = pool->begin();
         conn = it->second;
         if (! conn->IsClosed()) {
            this->CloseConn(conn);
         }
         
         delete conn;
      }
      
      Request * request;
      while(! requests->empty()) {
         request = requests->front();
         delete request;
         requests->pop();
      }
      
      if (codec != NULL) delete codec;
      
      delete transport;
      delete requests;
      delete pool;
   }
   
   Transport * Server::GetTransport() {
      return transport;
   }
   
   bool Server::HasConn(int i) {
      return pool->find(i) != pool->end();
   }
   
   // Find connection
   Connection * Server::FindConn(int i) {
      Pool::iterator p;
      p = pool->find(i);
      if (p != pool->end()) {
         return p->second;
      } else {
         return NULL;
      }
   }
   
   // Get connection instance
   Connection * Server::GetConn(int i) {
      Connection * conn = FindConn(i);
      assert(conn != NULL);
      
      return conn;
   }
   
   Connection * Server::OpenConn() {
      Connection * conn = new Connection();
      conn->server = this;
      conn->id = ++this->counter;
      
      pool->insert(std::pair<int, Connection*>(conn->id, conn));
      
      if (on_connect != 0) {
         on_connect(this, conn);
      } else {
         conn->SetCodec(codec);
      }
      
      return conn;
   }
   
   void Server::CloseConn(Connection * conn) {
      if (! HasConn(conn->id)) return;
      
      if (on_disconnect != 0) {
         on_disconnect(this, conn);
      }
      
      // TODO Mark requests as closed...
      pool->erase(conn->id);
      
      if (transport->HasConn(conn->id)) {
         transport->CloseConn(conn->id);
      }
      
      if (! conn->IsClosed()) {
         conn->Close();
      }
   }
   
   void Server::CloseConn(int i) {
      Connection * conn = this->GetConn(i);
      this->CloseConn(conn);
   }
   
   void Server::MaxConns(int i) {
      max_connections = i;
   }
   
   int Server::CountConns() {
      return pool->size();
   }
   
   void Server::OnConnect(ConnectionCallback callback) {
      on_connect = callback;
   }
   
   void Server::OnDisconnect(ConnectionCallback callback) {
      on_disconnect = callback;
   }
   
   
   
   
   int Server::CountRequests() {
      return requests->size();   
   }
   
   void Server::AddRequest(Request * req) {
      if (this->on_request == 0) {
         requests->push(req);
         return;
      }
      
      Response * res = new Response(req);
      // TODO Add remove callback res->OnSent([](Response * res){ delete res });
      on_request(req, res);
   }
   
   bool Server::HasRequest() {
      return requests->size() > 0;
   }
   
   Request * Server::GetRequest() {
      Request * req = requests->front();
      requests->pop();
      
      return req;
   }
   
   void Server::OnRequest(RequestCallback callback) {
      on_request = callback;
   }
   
   void Server::SendResponse(Response * res) {
      if (! is_started) return; // TODO Notifiy about error
      
      res->conn->Write(res->data);
      delete res;
   }
   
   int Server::Start() {
      is_started = true;
      return transport->Start();
   }
   
   int Server::Start(int port) {
      is_started = true;
      return transport->Start(port);
   }
   
   int Server::Start(const char * ip, int port) {
      is_started = true;
      return transport->Start(ip, port);
   }
   
   void Server::Stop() {
      if (! is_started) return;
      
      is_started = false;
      transport->Stop();
   }
   
   bool Server::IsStarted() {
      return is_started;
   }
   
   bool Server::IsRunning() {
      return transport->Loop();
   }
   
   // Codec  -------------------------------------------------------------------
   void Codec::Decode(Connection * conn) {
      size_t uint_size = sizeof(uint32_t);
      uint32_t chunk_size;
      
      while (conn->buffer.size() > uint_size) {
         memcpy(&chunk_size, (char *) &conn->buffer[0], uint_size);
         
         if (chunk_size > conn->buffer.size() + uint_size) break;
         
         conn->counter++;
         
         Request * req = new Request();
         Pack * pack = new Pack();
         
         req->id = conn->counter;
         req->conn = conn;
         req->data = (void *) pack;
         
         
         pack->data = (char *) malloc(sizeof(char) * chunk_size);
         pack->size = chunk_size;
         memcpy((char *) pack->data, (char *) &conn->buffer[uint_size], chunk_size);
         
         conn->buffer.erase(conn->buffer.begin(), conn->buffer.begin() + (chunk_size + uint_size));
         conn->server->AddRequest(req);
      }
   }
   
   void Codec::Encode(Connection * conn, void* data) {
      Pack * pack = reinterpret_cast<Pack *>(data);
      int size = sizeof(uint32_t) + sizeof(char) * pack->size;
      
      char * bytes = (char *) malloc(size);
      memcpy(bytes, (char*) &pack->size, sizeof(uint32_t));
      memcpy(bytes+sizeof(uint32_t), pack->data, pack->size);
      
      conn->Write(bytes, size);
      free(bytes);
   }
   
   void Codec::Clear(void * data) {
      Pack * pack = reinterpret_cast<Pack *>(data);
      delete pack;
   }
   
   // Request ------------------------------------------------------------------
   Request::~Request() {
      conn->GetCodec()->Clear(data);
   }
   
   Response::Response(Connection * conn) {
      this->conn = conn;
   }
   
   Response::Response(Request * request) {
      conn = request->conn;
   }
   
   Response::~Response() {
      conn->GetCodec()->Clear(data);
   }
   
   void Response::Send() {
      conn->Write(data);
   }
   
   void Response::Send(std::function<void(Response * res)> on_done) {
      conn->Write(data);
      on_done(this);
   }
   
   // Pack ---------------------------------------------------------------------
   Pack::Pack() {
      this->size = 0;   
   }
   
   Pack::Pack(const char* data, size_t size) {
      this->data = (char*) data;
      this->size = size;
   }
   
   Pack::~Pack() {
      free(this->data);   
   }
   
   // Connection ---------------------------------------------------------------
   Connection::Connection() {
      // buffer = new Buffer(buffer_size);   
   }
   
   Connection::~Connection() {
      // delete buffer;
   }
   
   bool Connection::IsClosed() {
      return is_closed;
   }
   
   void Connection::Close() {
      if (is_closed) return;
      
      is_closed = true;
      server->CloseConn(this);
   }
   
   void Connection::Push(char * data, size_t size) {
      std::copy(data, data + size, std::back_inserter(buffer));
      codec->Decode(this);
   }
   
   void Connection::Write(void* data) {
      codec->Encode(this, data);
   }
   
   void Connection::Write(const char* data, size_t size) {
      // char * msg = new char[size + 1];
      // memcpy(msg, data, size);
      // msg[size] = '\0';
      
      // printf("Write to connection #%i: %s\n", id, msg);
      // delete []msg;
      server->GetTransport()->Write(id, data, size);
   }
   
   void Connection::SetCodec(Codec * codec) {
      this->codec = codec;
   }
   
   Codec * Connection::GetCodec() {
      return codec;
   }
};