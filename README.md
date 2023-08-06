# ClientServerMMOFramework
MMO Client/Server networking in C++ using BOOST ASIO, Socket and Connections.

###  Internet connection example
- Simple ASIO client to server example using TCP/IP
  - Refer to `internet.cpp` for the code in the internet example directory.
- Has following challenges:
  - Complex
  - Not scalable
  - Not maintainable
  - Don't know how much data to expect from server (buffer size)

###  NOTE: Our aim is to create an MMO Client/Server networking framework that can handle all the challenges and offer a seamless solution that enables smooth message passing between the client and server.

