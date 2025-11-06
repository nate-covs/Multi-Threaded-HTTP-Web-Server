------------------------------------------------------------------------------------------------------------------------------------
🖥️ HTTP Web Server
------------------------------------------------------------------------------------------------------------------------------------
DISCLAIMER:
This project was created for educational purposes only. It is not intended for academic submission and should not be copied or used for coursework. Doing so may violate the CU Honor Code.
This repository is shared for recreational and learning use only.
-----------------------------------------------------------------------------------------------------------------------------------

📘 Description

This program implements a simple TCP-based web server capable of handling multiple concurrent HTTP GET requests from multiple clients.

Supported file extensions:

.html

.txt

.png

.gif

.jpg

.css

.js

-----------------------------------------------------------------------------------------------------------------------------------

⚙️ Building the Server

To build the server, run the following command in the project directory:

gcc server.c helper.c -o server -pthread

This will create an executable named server.

-----------------------------------------------------------------------------------------------------------------------------------

🚀 Running the Server

The server requires one argument: the port number on which it should listen.

Example usage:

./server < port >

-----------------------------------------------------------------------------------------------------------------------------------

🌐 Testing the Server

You can test the server using any modern web browser.

Navigate to:

http://localhost:<port>

The server will serve files from the www/ directory.

You can also connect with net cat and then make an http request like:

nc localhost <port> 

GET /index.html HTTP/1.1

-----------------------------------------------------------------------------------------------------------------------------------

🧹 Cleaning Up

To gracefully stop the server, press Ctrl+C.

The server will:

-Ensure all threads release their resources

-Close all active connections

-Free allocated memory 

-----------------------------------------------------------------------------------------------------------------------------------





