#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#define NAMING_SERVER_IP "127.0.0.1"

// typedef struct storage_server
// {
//     int sto_ser_num;
//     char ip_addr[15];
//     char nms_port[10];
//     char client_port[10];
//     int numb_files;
//     char **list_of_all_files;
//     struct storage_server *nextss;
// } storage_server;

typedef struct NS_to_client
{
    char client_port[10];
    char ip_address[15];
    char ded_nm_port[10];
    int ss_num;
} NS_to_client;

// typedef storage_server *ssptr;

int Connect_with_storage_server(char *ip, int port)
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        printf("ERROR 400 : Cannot Connect to the respective SS due to Socket Failure");
        return -1;
    }

    struct sockaddr_in storage_server_address;
    storage_server_address.sin_family = AF_INET;
    storage_server_address.sin_port = htons(port);
    storage_server_address.sin_addr.s_addr = inet_addr(ip);
    time_t start, end;
    double elapsed;
    start = time(NULL);
    while (connect(client_socket, (struct sockaddr *)&storage_server_address, sizeof(storage_server_address)) < 0)
    {
        end = time(NULL);
        elapsed = end - start;
        if (elapsed > 4)
        {
            // printf("\033[1;31mError 408 : Request Timeout\n\033[1;0m");
            return -1;
            break;
        }
        printf("\033[1;31mError 400 : Unable to connect to the Respected Storage Server\n\033[1;0m");
        printf("\033[1;37mTrying to Connect Again In 1 second\033[1;0m\n");
        sleep(1);
    }
    return client_socket;
}

int Connect_with_naming_server(int port)
{
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        printf("ERROR : Cannot Connect to the NS due to Socket Failure");
        return -1;
    }

    struct sockaddr_in naming_server_address;
    naming_server_address.sin_family = AF_INET;
    naming_server_address.sin_port = htons(port);
    naming_server_address.sin_addr.s_addr = inet_addr(NAMING_SERVER_IP);
    time_t start, end;
    double elapsed;
    start = time(NULL);
    while (connect(client_socket, (struct sockaddr *)&naming_server_address, sizeof(naming_server_address)) < 0)
    {
        
        end = time(NULL);
        elapsed = end - start;
        if (elapsed > 4)
        {
            // printf("\033[1;31mError 408 : Request Timeout\n\033[1;0m");
            return -1;
        }
        printf("\033[1;31mError 400 : Unable to connect to the Naming Server\n\033[1;0m");
        printf("\033[1;37mTrying to Connect Again In 1 second\n\033[1;0m");
        sleep(1);
    }
    return client_socket;
}

void read_info(char *path, int client_id_for_naming_server)
{
    // int client_id_for_naming_server = Connect_with_naming_server(client_id_for_naming_server);'
    int choice = 1;
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, path, strlen(path), 0);

    // ssptr found = (ssptr)malloc(sizeof(storage_server));
    NS_to_client found;
    memset(&found, 0, sizeof(found));
    if(recv(client_id_for_naming_server, &found, sizeof(found), 0)<=0)
    {
        printf("\033[1;31mERROR 406 : NS Disconnected\033[1;0m\n");
        return ;
    }
    // close(client_id_for_naming_server);
    if (found.ss_num == -1)
    {
        printf("\033[1;31mERROR 404 : No such File exists\033[1;0m\n"); // CHANGE MADE
        return;
    }
    int client_id_for_storage_server = Connect_with_storage_server(found.ip_address, atoi(found.client_port));
    if(client_id_for_storage_server==-1)
    {
        printf("\033[1;31mError 408: Request Timedout--Storage Server Not Available\033[1;0m\n");
        return;
    }
    send(client_id_for_storage_server, &choice, sizeof(choice), 0);
    send(client_id_for_storage_server, path, strlen(path), 0);
    int z;
    memset(&z,0,sizeof(z));
    if(recv(client_id_for_storage_server,&z,sizeof(z),0)<=0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\033[1;0m\n");
        return ;
    }
    int tons;
    if(z==1)
    {
    char data_recieved[1001];
    printf("\nThe Requested File has contents :\n\033[1;34m");
    memset(data_recieved, '\0', sizeof(data_recieved));
    while (1)
    {
        memset(data_recieved, '\0', sizeof(data_recieved));
        int len = recv(client_id_for_storage_server, data_recieved, 1e3, 0);
        if (len < 0)
        {
            if (len < 0)
            {
                perror("recieved failed");
            }
            break;
        }
        char *stop_ptr = strstr(data_recieved, "STOP");
        if (stop_ptr != NULL)
        {
            int stop_position = stop_ptr - data_recieved;

            for (int i = 0; i < stop_position; i++)
            {

                printf("%c", data_recieved[i]);
            }

            // printf("\n done\n");
            break; // STOP signal received
        }
        else
        {
            printf("%s\n", data_recieved);
        }
        memset(data_recieved, '\0', 1e3);
    }
    }
    close(client_id_for_storage_server);
    // char donereadingmessage[256];
    // memset(donereadingmessage, '\0', 256);
    if(z==1)
    {
        tons=1;
    }
    else if(z==-1)
    {
        tons=-1;
    }
    // strcpy(donereadingmessage, "ACK : STOP Recived While Reading\n");

    send(client_id_for_naming_server,&tons,sizeof(tons),0);
    char recvd_mssg[256];
    memset(recvd_mssg, '\0', sizeof(recvd_mssg)); // change made
    if (recv(client_id_for_naming_server, recvd_mssg, sizeof(recvd_mssg), 0) <= 0)
    {
        printf("\033[1;31mError 405 : Naming Server Disconnected\033[1;0m\n");
        // exit(EXIT_FAILURE);
    }
    printf("\033[1;32m\n%s\033[1;0m", recvd_mssg);
}

void write_info(char *path1, char *data_to_write, int client_id_for_naming_server)
{

    // int client_id_for_naming_server = Connect_with_naming_server(client_id_for_naming_server);
    int choice = 2;
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, path1, strlen(path1), 0);

    NS_to_client found;
    memset(&found, 0, sizeof(found));
    if(recv(client_id_for_naming_server, &found, sizeof(found), 0)<=0)
    {
        printf("\033[1;31mERROR 406 : NS Disconnected\n\033[1;0m");
        return ;
    }

    // close(client_id_for_naming_server);
    if (found.ss_num == -1)
    {
        printf("\033[1;31mERROR 404 : No such File exists\033[1;0m\n"); // CHANGE MADE
        return;
    }
    int client_id_for_storage_server = Connect_with_storage_server(found.ip_address, atoi(found.client_port)); // Not written the acknowledgement(STOP)
    if(client_id_for_storage_server==-1)
    {
        printf("\033[1;31mError 408: Request Timedout--Storage Server Not Available\033[1;0m\n");
        return;
    }
    send(client_id_for_storage_server, &choice, sizeof(choice), 0);
    char packet_to_send[10000];
    snprintf(packet_to_send, 10000, "%s;%s", path1, data_to_write);
    packet_to_send[strlen(packet_to_send)] = '\0';
    send(client_id_for_storage_server, packet_to_send, strlen(packet_to_send), 0);
    // send(client_id_for_storage_server, path, strlen(path), 0);
    // printf("%s\n", path);
    // printf("%s\n", data_to_write);
    // send(client_id_for_storage_server, data_to_write, strlen(data_to_write), 0);
    int z;
    memset(&z,0,sizeof(z));
    if(recv(client_id_for_storage_server,&z,sizeof(z),0)<=0)
    {
        printf("ERROR 405 : SS Disconnected\n");
        return ;
    }
    int tons;
    if(z==-1)
    {
        tons=-1;
    }
    else if(z==1)
    {
        tons=1;
    }
    close(client_id_for_storage_server);
    // char donereadingmessage[256];
    // memset(donereadingmessage, '\0', 256);

    // strcpy(donereadingmessage, "ACK : WRITE COMPLETED");

    send(client_id_for_naming_server,&tons,sizeof(tons),0);
    char recvd_mssg[256];
    memset(recvd_mssg, '\0', sizeof(recvd_mssg)); // change made
    if (recv(client_id_for_naming_server, recvd_mssg, sizeof(recvd_mssg), 0) <= 0)
    {
        printf("\033[1;31mError 405 : Naming Server Disconnected\033[1;0m\n");
        // exit(EXIT_FAILURE);
    }
    printf("\033[1;32m\n%s\033[1;0m", recvd_mssg);
}

void get_info(char *path, int client_id_for_naming_server)
{
    // int client_id_for_naming_server = Connect_with_naming_server(client_id_for_naming_server);
    int choice = 3;
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, path, strlen(path), 0);

    NS_to_client found;
    memset(&found, 0, sizeof(found));
    if(recv(client_id_for_naming_server, &found, sizeof(found), 0)<=0)
    {
        printf("\033[1;31mERROR 406 : NS Disconnected\033[1;0m\n");
        return ;
    }
    // close(client_id_for_naming_server);
    if (found.ss_num == -1)
    {
        printf("\033[1;31mERROR 404 : No such File exists\033[1;0m\n"); // CHANGE MADE
        return;
    }

    int client_id_for_storage_server = Connect_with_storage_server(found.ip_address, atoi(found.client_port));
    if(client_id_for_storage_server==-1)
    {
        printf("\033[1;31mError 408 : Request Timedout--Storage Server Not Available\033[1;0m\n");
        return;
    }
    send(client_id_for_storage_server, &choice, sizeof(choice), 0);
    send(client_id_for_storage_server, path, strlen(path), 0);
    int z;
    memset(&z,0,sizeof(z));
    if(recv(client_id_for_storage_server,&z,sizeof(z),0)<=0)
    {
        printf("\033[1;31mEERROR 405 : SS Disconnected\033[1;0mE\n");
    }
    if(z==1)
    {
    char permissions[1000];
    memset(permissions, '\0', sizeof(permissions));
    if(recv(client_id_for_storage_server, permissions, sizeof(permissions), 0)<=0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\n\033[1;0m");
        return ;
    }
    long long int file_sz;
    memset(&file_sz, 0, sizeof(file_sz));
    if(recv(client_id_for_storage_server, &file_sz, sizeof(file_sz), 0)<=0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\n\033[1;0m");
        return ;
    }
    close(client_id_for_storage_server);
    printf("\033[1;34mFile permissions :%s ", permissions);
    printf("%lld\n\033[1;0m", file_sz);
    }
    int tons;
    memset(&tons,0,sizeof(tons));
    // memset(donereadingmessage, '\0', 256);
    if(z==1)
    {
    // strcpy(donereadingmessage, "ACK: INFO RETRIEVED ");
    tons=1;
    }
    else if(z==-1)
    {   
        tons=-1;
        // strcpy(donereadingmessage, "ACK: INFO FAILED DUE TO NO PERMISSIONS ");
    }

    // send(client_id_for_naming_server, donereadingmessage, strlen(donereadingmessage), 0);
    send(client_id_for_naming_server,&tons,sizeof(tons),0);
    char recvd_mssg[256];
    memset(recvd_mssg, '\0', sizeof(recvd_mssg)); // change made
    if (recv(client_id_for_naming_server, recvd_mssg, sizeof(recvd_mssg), 0) <= 0)
    {
        printf("\033[1;31mError 405 : Naming Server Disconnected\033[1;0m\n");
        // exit(EXIT_FAILURE);
    }
    printf("\033[1;32m\n%s\033[1;0m", recvd_mssg);
}

void create(char *path, int client_id_for_naming_server, int isDirectory)
{
    // int client_id_for_naming_server = Connect_with_naming_server(client_id_for_naming_server);
    int choice = 4;
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, &isDirectory, sizeof(isDirectory), 0);
    send(client_id_for_naming_server, path, strlen(path), 0);
    char recvd_mssg[256];
    memset(recvd_mssg, '\0', sizeof(recvd_mssg)); // change made
    if (recv(client_id_for_naming_server, recvd_mssg, sizeof(recvd_mssg), 0) <= 0)
    {
        printf("\033[1;31mError 405 : Naming Server Disconnected\033[1;0m\n");
        return;
        // exit(EXIT_FAILURE);
    }
    printf("\033[1;32m\n%s\033[1;0m", recvd_mssg);
}
void delete(char *path, int client_id_for_naming_server)
{
    int choice = 5;
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, path, strlen(path), 0);

    char recvd_mssg[256];
    memset(recvd_mssg, '\0', sizeof(recvd_mssg)); // change made
    if (recv(client_id_for_naming_server, recvd_mssg, sizeof(recvd_mssg), 0) <= 0)
    {
        printf("\033[1;31mError 405 : Naming Server Disconnected\033[1;0m\n");
        return;
        // exit(EXIT_FAILURE);
    }
    printf("%s\n", recvd_mssg);
}
void copy(char *path_1, char *path_2, int client_id_for_naming_server)
{
    // int client_id_for_naming_server = Connect_with_naming_server(client_id_for_naming_server);
    int choice = 6;
    int k = 6;
    printf("client is sending copy request with source path %s and destination path %s\n", path_1, path_2);
    send(client_id_for_naming_server, &choice, sizeof(choice), 0);
    send(client_id_for_naming_server, path_1, 1000, 0);
    path_2[strlen(path_2)] = '\0';
    printf("M%ldM", send(client_id_for_naming_server, path_2, 1000, 0));
    fflush(stdout);
    char recieved_message[1000];
    memset(recieved_message, '\0', sizeof(recieved_message));
    if(recv(client_id_for_naming_server, recieved_message, sizeof(recieved_message), 0)<=0)
    {
        printf("ERROR 406 : NS Disconnected\n");
        return ;
    }
    if (strcmp(recieved_message, "ACK") == 0)
    {
        printf("Copy completed\n");
    }
    else
    {
        printf("Copy error\n");
    }
    // close(client_id_for_naming_server);
}
void clearScreen()
{
    printf("\033[2J\033[1;1H"); // ANSI escape code for clearing the screen
}
int main(int argc, char *argv[])
{

    while (1)
    {
        clearScreen(); // Clear the screen before displaying the menu

        printf("\033[1;33m**********************************************\n");
        printf("              Welcome to My Program           \n");
        printf("**********************************************\n");
        printf("Enter:\n");
        printf("1 for Read\n");
        printf("2 for Write\n");
        printf("3 for Get Info\n");
        printf("4 for Create\n");
        printf("5 for Delete\n");
        printf("6 for Copy\n");
        printf("7 for Exit\n\n\033[1;37m");
        int choice;
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
        {
            clearScreen();
            printf("READ\n");
            // Perform read operation here
            char path[1000];
            printf("\033[1;33mEnter the path of the file you want to read\033[1;37m\n");
            scanf("%s", path);
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
               printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            read_info(path, client_socket);
            close(client_socket);
            break;
        }
        case 2:
        {
            clearScreen();
            printf("WRITE\n");
            // Perform write operation here
            char path[1000];
            getchar();
            printf("\033[1;33mEnter the path of the file you want to write to\033[1;37m\n");
            fgets(path, sizeof(path), stdin);
            char *newline = strrchr(path, '\n');
            if (newline != NULL)
            {
                *newline = '\0';
            }
            char data_to_write[1000];
            printf("\033[1;33mEnter the text you want to write to the file\033[1;37m\n");
            fgets(data_to_write, sizeof(data_to_write), stdin);
            char *check_line = strrchr(data_to_write, '\n');
            if (check_line != NULL)
            {
                *check_line = '\0';
            }
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
                printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            write_info(path, data_to_write, client_socket);
            close(client_socket);

            break;
        }
        case 3:
        {
            clearScreen();
            printf("GET INFO\n");
            // Perform get info operation here
            char path[1000];
            printf("\033[1;33mEnter the path of the file for which you want to retrieve information\033[1;37m\n");
            scanf("%s", path);
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
                printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            get_info(path, client_socket);
            close(client_socket);

            break;
        }
        case 4:
        {

            clearScreen();
            printf("CREATE\n");
            // Perform create operation here based on isDirectory
            int isDirectory = 0;
            printf("\033[1;33mEnter 1 for Creating Directory or 0 for Creating a File\033[1;37m\n");
            scanf("%d", &isDirectory);
            char path[1000];
            if (isDirectory == 0)
            {
                printf("\033[1;33mEnter the name of file to be created\033[1;37m\n");
            }
            else if (isDirectory == 1)
            {
                printf("\033[1;33mEnter the name of directory to be created\033[1;37m\n");
            }
            scanf("%s", path);
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
                printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            create(path, client_socket, isDirectory);
            close(client_socket);

            break;
        }
        case 5:
        {
            clearScreen();
            printf("DELETE\n");
            // Perform delete operation here
            char path[1000];
            printf("\033[1;33mEnter the name of the directory/file to be deleted\033[1;37m\n");
            scanf("%s", path);
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
                printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            delete (path, client_socket);
            close(client_socket);

            break;
        }
        case 6:
        {
            clearScreen();
            printf("COPY\n");
            // Perform copy operation here
            char path_1[1000];
            char path_2[1000];
            printf("\033[1;33mEnter source path\033[1;37m\n");
            scanf("%s", path_1);
            printf("\033[1;33mEnter destination path\033[1;37m\n");
            scanf("%s", path_2);
            int client_socket = Connect_with_naming_server(atoi(argv[1]));
            if (client_socket == -1)
            {
               printf("\033[1;31mError 408: Request Timedout--Naming Server Not Available\033[1;0m\n");
                break;
            }
            copy(path_1, path_2, client_socket);
            close(client_socket);

            break;
        }
        case 7:
        {
            clearScreen();
            printf("Exiting...\n");
            sleep(2); // Wait for 2 seconds before exiting
            return 0;
        }
        default:
        {
            clearScreen();
            printf("\033[1;31mINVALID INPUT\033[1;37m\n");
            break;
        }
        }

        printf("\033[1;33m\nPress Enter to continue...\033[1;37m");
        while (getchar() != '\n')
            ;      // Clear input buffer
        getchar(); // Wait for user input before displaying the menu again
    }
}
