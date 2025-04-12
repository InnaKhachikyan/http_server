Task steps:
1) Write the server c  file
2) Write the client c file
3) Receive a message from the client
4) Parse the message: request, host, basicauth 
5) decode the user and password
6) verify with the table the user/password
7) check if the given path exists, return whatever is in the file, else return 404 error.
