nginx thrift module example
============
* Example nginx module that gets content (photo in this module) from external thrift server, and response to client (browser) 

* Why I wrote this
	1. People keep trying to write http server to serve content from thrift server using various libraries (proxygen, libevent...) but they forget nginx always perform better 
	2. it's difficult to learn nginx module developent at first, especially mixing C/C++ in a nginx module (when using thrift)  
	3. Learning

* This module using Apache thrift, boost, and ConncurrentQueue.hpp (to create connection pool to thrift server), to install it your system must have thrift, and boost[system, thread) installed  
* You cannot just install this module, you have to write a thrift server, then modify this module to meet your need. 

Usage
=====

To compile into nginx

::

    ./configure --with-ld-opt="-lstdc++" --add-module=nginx_photo_thrift_module     

I wrote this module using nginx-1.7.8

Modify
============

- This module takes 5 configuration directives from nginx.conf: 
    - ``thrift`` to check if this module enabled or not
    - ``content_server_address`` IP address of thrift server that serve photo content (the photo itself).
    - ``content_server_port`` port of thrift server above.
    - ``meta_server_address``  IP address of thrift server that serve photo metadata
    - ``meta_server_port``  port of thrift server above. 

- photodb_client.cpp is thrift client written in C++ (base on it, you should write another thrift client easily)
- modify ngx_http_photo_thrift_module.h, ngx_http_photo_thrift_module.c, photodb_client.h 


License
=======

@Ethic Consultant 2014
`GPL <http://www.gnu.org/licenses/gpl-3.0.txt>`_
