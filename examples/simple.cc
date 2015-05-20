#include <iostream>
#include <assert.h>
#include <cstring>
#include <string>
#include <functional>
#include "../poe.h"

void push_uint32(char * buf, uint32_t i) {
    memcpy(buf, &i, sizeof(uint32_t));
}

void push_bytes(char * buf, const char * data, size_t size) {
    memcpy(buf, data, size);
}

struct count_t {
    int c;
};

using namespace Poe;

int main() {
    count_t counter = {0};
    
    Server * server = new Server();
    
    // Server * server = new Server([&counter](Request * req, Response * res){
    //     counter.c++;
        
    //     Pack * pack = new Pack();
        
    //     pack->data = (char *) malloc(sizeof(char) * 5);
    //     memcpy(pack->data, (char*)"Hello", 5);
    //     pack->size = 5;
        
    //     res->data = (void *) pack;
    //     res->Send([](Response * res){
    //         delete res;
    //     });
        
    //     delete req;
    // });
    
    Connection * conn = server->OpenConn();
    server->OpenConn();
    server->OpenConn();
    
    size_t uint_size = sizeof(uint32_t);
    const char * ok = "ok";
    size_t size = uint_size + sizeof(char) * strlen(ok);
    char * message = (char*) malloc(size);
    push_uint32(message, (uint32_t) strlen(ok));
    push_bytes(message+uint_size, ok, strlen(ok));
    
    // std::cout << strlen(ok) << ":" << size << std::endl;
    // for(int i = 0; i < size; i++) {
    //     std::cout << message[i];
    // }
    // std::cout << std::endl;
    
    // uint32_t read_size;
    // memcpy(&read_size, message, uint_size);
    // std::cout << read_size << std::endl;
    
    // Response * response = new Response(conn);
    // response->bytes = "hello";
    // response->size = 5;
    //
    // Pack pack = new Pack();
    // pack->data = (char *) malloc(sizeof(char) * length);
    // pack->size = buffer.size();
    // std::copy(pack->data, (char *) &buffer[0], pack->size);
    //
    // conn->Write(pack);
    
    server->Start(7000);
    
    // conn->Push(message, size);
    // conn->Push(message, size);
    // conn->Push(message, size);
    free(message);
    
    // printf("Requests processed %i\n", counter.c);
    
    // assert(server->CountRequests() == 3);
    int i = 3;
    // int x = 100000000000;
    while (server->IsRunning()) { // Run underlay event loop
        while (server->HasRequest()) {
            Request * req = server->GetRequest();
            Response * res = new Response(req);
            Pack * pack = new Pack();
            Pack * reqPack = (Pack *) req->data;
            
            pack->data = (char *) malloc(sizeof(char) * reqPack->size);
            memcpy(pack->data, reqPack->data, reqPack->size);
            pack->size = reqPack->size;
            
            res->data = (void *) pack;
            res->Send([](Response * res){
                delete res;
            });
            
            i--;
            // delete res;
            delete req;
            printf("## %i\n", i);
        }
        
        if (i == 0) server->Stop();
    }
    
    
    
    // assert(server->CountRequests() == 0);
    
    delete server;
}