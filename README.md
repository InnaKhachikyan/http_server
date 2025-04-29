Task steps:
1) Write the server c  file
2) Fork/thread for each client request
3) Receive a message from the client
4) parse http request from client (headers which include auth info, request method, host, path, etc.)
5) decode the user and password
6) verify with the table the user/password
7) check if the given path exists, return whatever is in the file, else return 404 error.
8) Return the response unchunked.

Besides C compiler also requires OpenSSL development libraries.

## How to test
-make (to build)
-make test (to run the tests with the script)
I have a script which contains tests for the server.

I have also pushed the folder www, which contains index.html, css and js files, so that the server may be checked in the browser.

## Files

### server.c
The main functionality is implemented here.
### userpass_map.h
Headers for the user passwords map. Gives the main "interface" to the user without exposing the implementation details.
### userpass_map.c
Main implementation of the functions for the map, add/delete/authenticate/init etc.
Initially these two were in the same .c file, however I had error "multiple definition", so I had to separate the headers with ifndef (if defined not to redefine it).

## Features:
- Handling multiple connections: the server listens on port 8080, polls for connections.
- Forks for each client: the process is forked for each connection with a client
- Method Restrictions: handles only GET method, for any other method sends 405, Method not allowed.
- Basic Authentification: uses openSSL for base64 decoding
- Index files: if the request path ends with a trailing ‘/’ (for example /, /docs/, /img/gallery/), the server appends index.html to that path before attempting to serve the file.
- Directory traversal protection: Rejects any path containing "..", sends Bad request.
- Query parameters: are striped, the parsing is done up to "?" or newline.
- Body handling: the body of the request (if there is any) is ignored.

### main():
the user map is initialized and populated with a set of username/password pairs. A listening socket is created on PORT (default 8080).

### poll_for_connections():
monitors the listening socket using poll() with wait time 100ms, when ready, accepts the client socket and calls the fork_for_client function.

### fork_for_client
pid is used to check the fork of the process. If it's negative, then something went wrong, if it's 0, then we are in the child process and don't need the listening socket anymore, close it, and call the handle_client_request method. If it's 1, then we are in parent process and don't need the client socket anymore, close it.

### read_http_request
Reads the request in READ_CHUNK chunks in a while loop reallocating each time the buffer when the next chunk needs to be read Stops when the size is too big (MAX_HEADER_SIZE) or when the "\r\n\r\n" is reached. If fails, frees allocated memory and eiter returns -1 or calls exit().

### trim
Eliminated leading/trailing whitespaces from the string in-place

### parse_http_request
Splits on \r\n, then tokenizes the request-line into method, path and version. Strips any query string from the path (anything after "?"). Stores each header name/value pair in the req->header_name/req->header_value arrays. Captures any body after \r\n\r\n. Returns 0 on success and -1 on malformed input.
An entirely empty path is not valid, real clients mostly send at least '/', so in that case the parser returns -1.

### authenticate
Searches for the corresponding header and assigns to auth var.
Errors:
400, Bad request: 
	- If not starting with "Basic "
	- If decoding went wrong and not a positive number returned
	- If format user:pass missing
401, Unauthorized:
	- If Authorization header is missing
403, Forbidden:
	- If user authentication failed (wrong username or password)
Returns true if authentication succeeds.

### send_file_response
If the method is not GET, sends 405 Method not allowed.
If the client tries to enter parent directory with "..", 400, Bad request is sent.
If the request path is '/' or ends with a trailing '/', the server treats it as a directory and appends index.html before trying to serve the file.
If anything goes wrong when opening the file, 500, Internal Server Error is sent. Otherwise, 200, OK.
If the file is not there, 404, Not found.

### handle_client_request()
In the child this method is called, it calls corresponding methods in the body to do the job step by step:
1) read_http_request: if negative return, closes the socket, returns.
2) parse_http_reqeust: if negative return, frees memory, closes the socket, returns.
3) authenticate: if went wrong, frees memory and returns.
4) send_file_response
5) in the end calls the cleanup and closes the client socket.

### cleanup()
frees all dynamically allocated memory and closes the socket.

P.S. I know it would be better to have http_response as a separate struct, the code would be more readable, scalable and cleaner. But did not have much time to update the implementation accordingly.
