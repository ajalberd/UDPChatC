In this base directory there are all the files neccesary to compile and run the client.
In the directory ./server/ there are all the files neccesary to compine and run the server.

I have tested this on the latest gcc in Linux. It should compile and run on most modern Linux machines.

Just run 'make' in both directories, and then run both client and server with these arguments in seperate terminals:

Assuming base directory:
./server/server 5000 127.0.0.1
         Port  Address

./client 5000 127.0.0.1 NameOfFile

The server should send any file to the client as long as the file is in the ./server/ directory.

I included a testfile 'File' in the Server directory to use, although any file should work. I have tested jpg files size 1Mb and they work as well.
