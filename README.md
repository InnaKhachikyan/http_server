Task steps:
1) Write the server c  file
2) Fork/thread for each client request
3) Receive a message from the client
4) parse http request from client (headers which include auth info, request method, host, path, etc.)
5) decode the user and password
6) verify with the table the user/password
7) check if the given path exists, return whatever is in the file, else return 404 error.
8) Handle chunked and unchunked server response features.
