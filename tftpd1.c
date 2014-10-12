#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include <strings.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

// Read request (RRQ)
#define TFTP_RRQ    (1)     

// Write request (WRQ)
#define TFTP_WRQ    (2)     

// Data (DATA)
#define TFTP_DATA   (3)

    
// Acknowledgment (ACK)
#define TFTP_ACK    (4)

// Error (ERROR)
#define TFTP_ERROR  (5)



void err_exit() {
  perror ("My Server");
  exit(1);
}


int main(int argc, char** argv) 
{
    int port = 8888; // atoi(argv[1]);
    int sockfd;
    int acksockfd;
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    int yes=1;        // for setsockopt() SO_REUSEADDR, below


    const int BUFFER_SIZE = 512;
    char  client_buffer[ BUFFER_SIZE ];
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);
    

    if((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        err_exit();
    
    
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    
    socklen_t client_len = sizeof( sizeof(clientaddr) );
    
    
  // set every value in structure to zero then overwrite the ones that we used to specify.
  // set rest of the fields to zero.

  bzero(&serveraddr, sizeof(serveraddr));  
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
  serveraddr.sin_port = htons(port); // host to network byte order, 8888:  port number is a short: 16 bit-value
  // converts byte order to allow communication between machines with different byte ordering.
  // different comps have different byte orders.

  // machines talk in byte orders.

  if(bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0)
    err_exit();
    
    FD_SET(sockfd, &master);
    // keep track of the biggest file descriptor
    fdmax = sockfd; // so far, it's this one
    


    int i = 0;

  for (;;) {
      int client_socket;
      read_fds = master; // copy it
      char client_ip_addr[ INET_ADDRSTRLEN ];
      int data_len = 0;
      char tftp_mode[20];

      if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
          err_exit();
      }
      
      int connections;
      // run through the existing connections looking for data to read
      for(connections = 0; connections <= fdmax; connections++) {
          if (FD_ISSET(connections, &read_fds)) { // we got one!!
              if (connections == sockfd) {
                  client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                  FD_SET(client_socket, &master); // add to master set
                  if (client_socket > fdmax) {    // keep track of the max
                      fdmax = client_socket;
                  }
              }
          }
      else{
          // handle data from a client
          if((data_len = recvfrom(sockfd, client_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &client_len))<=0){
     FD_CLR(connections, &master); // remove from master set
          } else {
                sleep(6);
                char file_name[100];
                int  opcode = 0;
                int count = 2;
                int offset = 0;
                FILE* fp = NULL;
                int num_bytes_read = 0;
                char file_read_buffer[512+4]; // 4 bytes for the header
                // char last_data_packet[516]
                unsigned short block_id = 0;      

                if (data_len == -1)
                    err_exit();
      
      
                printf("Received a Connection\n");
            inet_ntop(AF_INET, ((struct sockaddr_in *) &clientaddr), client_ip_addr, INET_ADDRSTRLEN);
            printf("Client IP: %s\n", inet_ntoa(clientaddr.sin_addr));
             printf("Client Port: %d\n", ntohs(clientaddr.sin_port));


                printf("Message Received: ");
                for( i = 0; i < data_len; i++ ) {
                    printf("%2x ", client_buffer[i]);
                }
      
                printf("\n");
                opcode = (client_buffer[0] << 8) | client_buffer[1] ;

      
                switch( opcode ) {
                    case TFTP_RRQ:
                        printf("OPCODE(%d): TFTP RRQ Received\n", opcode);
                        break;
              
                    case TFTP_WRQ:
                        printf("OPCODE(%d): TFTP WRQ Received\n", opcode);
                        break;
            
                    case TFTP_DATA:
                        printf("OPCODE(%d): TFTP DATA Received\n", opcode);
                        break;     
              
                    case TFTP_ACK:
                        printf("OPCODE(%d): TFTP ACK Received\n", opcode);
                        break;
              
                    case TFTP_ERROR:
                        printf("OPCODE(%d): TFTP ERROR Received\n", opcode);
                        break;    
              
                    default:
                        printf("ERROR(%d): Unknown TFTP OPCODE Received\n", opcode);
                }

      
                // We start looking for filename from loc == 2 
                // because, first two bytes are taken by opcode
                offset = 2;
                count = offset;
      
                // Extract File Name
                while( client_buffer[count] != '\0') {
                    file_name[count - offset] = client_buffer[count];
                    ++count;
                }
      
                file_name[count - offset] = '\0';
      
                printf("File Name: %s\n", file_name);
      
                ++count;
      
                offset = count;
      
                // Extract File Transfer Mode
                while( client_buffer[count] != '\0') {
                    tftp_mode[count - offset] = client_buffer[count];
                    ++count;
                }    
                tftp_mode[count - offset] = '\0';
                printf("Transfer Mode: %s\n\n", tftp_mode);  
                fp = fopen( file_name, "rb");
                if( fp != NULL) {
                    
                    num_bytes_read = 512;
                    block_id = 1;
          
                    while(num_bytes_read == 512) {
                        printf("Sending BLOCK#%d", block_id);
                        num_bytes_read = fread((void*) file_read_buffer + 4, sizeof(char), 512, fp);
              
                        // Create a data packet
                        *((unsigned short*)(file_read_buffer + 0)) = htons(TFTP_DATA);
                        *((unsigned short*)(file_read_buffer + 2)) = htons(block_id);
              
                        block_id += 1;
              
              
              
                        printf(" BLOCK Size: %d\n", num_bytes_read);
              
                        // Now send the Message to Client
                        sendto(client_socket, file_read_buffer ,4 + num_bytes_read, 0,(struct sockaddr *) &clientaddr, client_len);
              
#if 0
              printf("Waiting for ACK\n");
              
              if((acksockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
                  err_exit();
              
              // Now wait for the ack
              data_len = recvfrom(acksockfd, client_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &client_len);
              
              // now ignore the ack :)
              if( data_len == 4) {
                  printf("TFTP ACK Received\n");
              }
              else {
                  printf("Was expecting ACK, but received something of len = %d\n", data_len);
              }
#endif
              
                    }
                    printf("File Sent Successfully\n");
                }
                else {
                    // Create the error message
                    char errorMessage[100];
                    char errorString[] = "File Not Found\0";
                    errorMessage[0] = 0;
                    errorMessage[1] = 5;
          
                    (*(unsigned short*)&errorMessage[2]) = htons(1);
                    memcpy( errorMessage + 4, errorString, strlen(errorString) );
                    errorMessage[4 + strlen(errorString)] = 0;    
          
                    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
          
                    // Now send the Error Message to Client
                    sendto( client_socket, errorMessage ,5 + strlen(errorString), 0,(struct sockaddr *) &clientaddr, client_len);              
                }
                FD_CLR(connections, &master);
                shutdown(connections,2);
              shutdown(sockfd,1);
          }
      }
      }
      }
}
  //}