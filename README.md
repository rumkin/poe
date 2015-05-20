Poe is experimental data transport with customizable protocol. It allow to support
different data encodings and to switch protocol on the fly. It use asynchronous
mode.

Callback based example:

```cpp
Poe::Server * server = new Poe::Server([](Poe::Request * req, Poe::Response * res){
    res.Write("Hello", 5);
});

server->Start(7000); // Start listening localhost port
```

For example it allow to use some text protocol for connection establish and then
to awitch connection to use HTTP:

```cpp
Poe::Server * server = new Poe::Server();

server->OnConnect([](Poe::Server * server, Poe::Connection * conn) {
    conn->SetCodec(new Poe::Codec);
});

server->OnDisconnect([](Poe::Server * server, Poe::Connection * conn) {
    delete conn->GetCodec();
});

server->Start(7000);

while(server->IsRunning()) {
    while(server->HasRequest()) {
        Poe::Request req = server->GetRequest();
        
        if (req->conn->counter == 1) { // Switch after the first request
            Poe::Pack * pack = reinterpret_cast<Poe::Pack*>(req->data);
            
            std::string str(pack->data, pack->size);
            
            if (str.compare("secret-value") != 0) {
                req->conn->Close();
                break;
            }
            
            // Switch codec
            delete req->conn->GetCodec();
            req->conn->SetCodec(new HttpCodec());
        } else {
            HttpData * http = reinterpret_cast<HttpData*>(req->data);
            
            // Do something with HttpPackage
            http->OnData([](char * data, size_t len) {
                // Do something with http data
            });
            
            http->OnEnd([](HttpData * data) {
                delete data;
            });
        }
        
        delete req; // Destroy request object when unnecessary
    }
}

```

While HttpCodec is not implemented yet it only support default fixed length
data protocol encoding. In the future it became streaming and will support 
nested codecs to support multiple protocols incapsulation. For example
pack HTML or video stream into CBOR which wrapped into fixed length encoding.

## Building

Fetch and build deps:

```bash
make deps # Fetch dependencies
make build
```

Or build example:

```bash
make deps
make example
```