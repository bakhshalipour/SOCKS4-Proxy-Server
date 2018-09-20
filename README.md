# SOCKS4-Proxy-Server

This project implements a proxy server using [SOCKS 4](https://en.wikipedia.org/wiki/SOCKS#SOCKS4) and [SOCKS 4A](https://en.wikipedia.org/wiki/SOCKS#SOCKS4a) protocols. A proxy server is a server which operates as a go-between for clients querying resources from other servers. 

The code is heavily-multithreaded and optimized for maximizing throughput while maintaining flexibility. 

## Usage

### Compile 

    $ make

### Run 
    $ ./server.out [port]
Once the command is issued, the server starts listening on the specified port. 

## License
    Copyright © 2015-2018 Mohammad Bakhshalipour

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
    the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see {http://www.gnu.org/licenses/}.
    
