# SOCKS4-Proxy-Server

This project implements a proxy server using [SOCKS 4](https://en.wikipedia.org/wiki/SOCKS#SOCKS4) and [SOCKS 4A](https://en.wikipedia.org/wiki/SOCKS#SOCKS4a) protocols. A proxy server is a server which operates as a go-between for clients querying resources from other servers. 

The program can be used for bypassing the [Internet Censorship](https://en.wikipedia.org/wiki/Internet_censorship_in_Iran). The code is heavily-multithreaded and optimized for maximizing throughput while maintaining flexibility. 

## Usage

### Compile 

    $ make

### Run 
    $ ./server.out [port]
One the command is issued, the server starts listening on the specified port. 
