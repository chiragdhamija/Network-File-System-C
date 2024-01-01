#### REPORT
### How to Run the code

Firstly we initialise the NM server by running ns.c and giving universal storage server port and universal client port as command line arguments. 
To initialise the SS we run ss.c and then copy the executable in the respective SS folders and give the NM server port number ,dedicated port number for later connection with naming server and dedicated port number for connection with clients as command line arguments. 
We run the client.c to start taking input for the functionalities and give NM server port number as command line arguments.

#### Functionalities and their Implementations

## 1. Read 
Client can request to read a file by giving choice as 1 and giving the path to that file. The request is sent to  the NM server where it finds the particular SS in which that file is stored and return the SS.port and SS.ip_address to the client. Then the client makes direct connection with the storage server using the SS.port and SS.ip_address and we are reading the data in chunks(to handle files with larger size than the RAM) and sending it to the client. The “STOP” packet serves as a signal to conclude the operation. We use the read command to read from the file.

## 2. Write : 
Client can request to write in a file by giving choice as 2 and giving the path to the file and data to write. Similar to Read , the request is sent to the NM server where it finds the particular SS in which that file is stored and return the SS.port and SS.ip_address to the client. The client makes direct connection with SS and send the data to write to SS and SS writes the data to that particular file. We use fwrite to write in the file. The SS sends acknowledgement to client on completion of the task.

## 3. Get Info : 
Similar Implementation as Read and Copy.We get permissions of the file and size of the file. We get permissions and size of the file using struct stat file.The SS sends acknowledgement to client on completion of the task.

## 4. Create :  
Client can request to create a file or directory  by giving choice as 4 and giving the path and telling whether they want to create a file or directory. The request is sent to the NM server where it finds the SS in which the path in which new file or directory is being created is stored and then connectes to the respective SS and SS creates the directory using mkdir command and file name using fopen in write flag which creates the file. On completion of task Acknowledgement is sent to the NM which NM forwards to the Client.

## 5. Delete :
Client can request to delete a file or directory by giving choice as 5 and giving the path of the file or directory. The request is sent to NM where it finds the SS in which the file or directory is stored . Then it makes a connection with the particular SS and send it the request to delete the file. The SS deletes the file or directory using rm -r <path> command. 

## 6. Copy :
Client can request to copy files or directories by giving choice as 6 and giving the source and the destination paths.We have implemented two types  a) Src and Dest in same SS  b) Src and Dest in different SS. The client sends the request to the Naming Server and it takes the responsibility to complete the task by making connections with the SS. Upon the successful completion of the copy operation, the NM sends an acknowledgment (ACK) packet to the client. As we are copying in chunks so any size files or directories can be copied.

### Assumptions in Client operations

1. While creating the file we take the input from the user whether the user want to make a directory or a file.
2. Cannot create a file or directory in the root directory.
3. Only read and write concurrency handled. While doing create , delete or copy any other functionality cannot be implemented on the same file.
4. As we are using TCP so the initial Acknowledgements between the Naming Server and clients are done with the threeway Handshake. The final ACKs are handled by us.
5. While giving the accessible paths of a SS 

 ## Adding New Storage Server 
In our implementation a new storage server can be added to the entries of the Naming Server at any point during execution.

## Storing Storage Server Data
Naming Server stores the critical information about the Storage Servers connected with it.

## Client Task Feedback
NM provides feedback to the requesting  clients on completion of their request task.

## Concurrent Client Access
In our implementation multiple clients can access the Naming Server simultaneously. Also ,NM responds to client requests with an initial ACK to acknowledge the receipt of the request. If this initial ACK is not received within a reasonable timeframe, the client may display a timeout message.

## Concurrent File Reading
Only one client can do write operations on a file at a given time while multiple clients can read the same file at a given time. If some clients are 

## Error Codes
 	400 : unable to connect to naming server 
 	408 : request timeoutb
	503 : NS not available
 	404 : no such file or directory exists
 	405 : SS disconnected
 	301 : client disconnected
 	406 : NS disconnected
 This is the set of error codes that can be returned when a client’s request cannot be accommodated.

 ## Efficient Search
 We are using tries to identify the correct Storage Server(SS) for a given request.This optimization enhances response times, especially in systems with a large number of files and folders.

 ## LRU Caching
 We have also implemented LRU Caching by using which NM can expedite subsequent requests for the same data, further improving response times and system efficiency.

 ## Failure Detection
 NM can detect Storage Server failures.

 ## SS recovery
  When an SS comes back online (reconnects to the NM), the duplicated stores should be matched back to the original SS. Additionally, no new entries should be added to the list of accessible files and folders in the NM during this process.

## Logging And Message Display
 Implemented a logging mechanism where the NM records every request or acknowledgment received from clients or Storage Servers.NM display or print relevant messages indicating the status and outcome of each operation.

 ## IP Address and Port Recording: 
 The log include relevant information such as IP addresses and ports used in each communication.


                 
ns.c is Naming Server Code. sto.c is Storage Server Code. client.c is Client Code.
    


