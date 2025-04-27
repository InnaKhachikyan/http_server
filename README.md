Task steps:
1) Write the server c  file
2) Fork/thread for each client request
3) Receive a message from the client
4) parse http request from client (headers which include auth info, request method, host, path, etc.)
5) decode the user and password
6) verify with the table the user/password
7) check if the given path exists, return whatever is in the file, else return 404 error.
8) Handle chunked and unchunked server response features.

Currently I implemented the struct hhtp_request and the method read_http_request in a way that those would handle any length up to MAX_HEADER_SIZE, the standard deviation is 8KB-16KB, so Ijust chose 8 * 1024.


Users: I implemented a small separate file for storing user-pass table. We have an array of struct, where we can add, delete, search and authenticate. The array is dynamic: if we reached the capacity, it resizes.
Currently I am just including my userpass_map.c file in the server.c, but it gives error of multiple definition, so I will have to separate header + implementation of the userpass_map.

