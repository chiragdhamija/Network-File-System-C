#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct filesll
{
    char name[256];
    int reader_count;
    pthread_mutex_t mutex;
    pthread_mutex_t write_mutex;
    struct filesll *nxt;
} filesll;
typedef struct filesll *filesllptr;
FILE *book;

typedef struct NS_to_client
{
    char client_port[10];
    char ip_address[15];
    char ded_nm_port[10];
    int ss_num;
} NS_to_client;

typedef struct recievedstruct // struct used to send to naming server for a particular ss info
{
    char ip_address[15];
    char NM_port[10];
    char Client_port[10];
    int num_files;
    char all_files[10000];
} recievedstruct;

typedef struct storage_server
{
    int sto_ser_num;
    char ip_addr[15];
    char nms_port[10];
    char client_port[10];
    int numb_files;
    char list_of_all_files[10000];
    int r1;
    int r2;
    int backed_up;
    int avaliable;
    struct storage_server *nextss;
} storage_server;
typedef storage_server *ssptr;

typedef struct lru
{
    char ip_address[15];
    char Client_port[10];
    char input[4097];
    int ss_num;
    char ded_nm_port[10];
    struct lru *nextlruelem;
} lru;
typedef lru *lruptr;

typedef struct trienode
{
    struct trienode *children[95]; // 136-32+1
    int flag;
    int ss_num;
} trienode;

typedef struct trienode *TriePtr;

TriePtr T;
ssptr storageserverhead;
int num_stor = 0;
int num_online_stor = 0;
int lrulen = 0;
lruptr lruhead;
filesllptr fileshead;
pthread_mutex_t sslllock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t numstorlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trielock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lrulock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t allfileslock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t booklock = PTHREAD_MUTEX_INITIALIZER;

filesllptr createnewfilenodell(char *input)
{
    filesllptr new = (filesllptr)malloc(sizeof(filesll));
    strcpy(new->name, input);
    new->reader_count = 0;
    if (pthread_mutex_init(&new->mutex, NULL) != 0)
    {
        perror("Mutex init failed");
    }
    if (pthread_mutex_init(&new->write_mutex, NULL) != 0)
    {
        perror("Mutex init failed");
    }
    return new;
}

void insertfile(filesllptr insert)
{
    if (fileshead->nxt == NULL)
    {
        fileshead->nxt = insert;
    }
    else
    {
        filesllptr prev = fileshead->nxt;
        fileshead->nxt = insert;
        insert->nxt = prev;
    }
}

void deleteafile(char *path)
{
    filesllptr temp = fileshead;
    filesllptr prev = temp;
    temp = temp->nxt;
    int found = 0;
    while (temp != NULL)
    {
        if (strcmp(temp->name, path) == 0)
        {
            found = 1;
            prev->nxt = temp->nxt;
            temp->nxt = NULL;
            pthread_mutex_destroy(&temp->mutex);
            pthread_mutex_destroy(&temp->write_mutex);
            free(temp);
            break;
        }
        prev = temp;
        temp = temp->nxt;
    }
}
lruptr createlrunode(int ss_num, char *inp, char *cp, char *ip, char *dnmp)
{
    lruptr new = (lruptr)malloc(sizeof(lru));
    new->ss_num = ss_num;
    strcpy(new->Client_port, cp);
    strcpy(new->input, inp);
    strcpy(new->ded_nm_port, dnmp);
    strcpy(new->ip_address, ip);
    return new;
}

void insertnewlrunode(lruptr ins)
{

    if (lruhead->nextlruelem == NULL)
    {
        lruhead->nextlruelem = ins;
        lrulen++;
    }
    else
    {
        lruptr prev = lruhead->nextlruelem;
        lruhead->nextlruelem = ins;
        ins->nextlruelem = prev;
        lrulen++;
    }
    if (lrulen > 15)
    {
        lruptr temp = lruhead;
        lruptr prev_del;
        temp = temp->nextlruelem;
        while (temp->nextlruelem != NULL)
        {
            prev_del = temp;
            temp = temp->nextlruelem;
        }
        prev_del->nextlruelem = NULL;
        free(temp);
        lrulen--;
    }
}
void deletelrunode(char *input)
{
    lruptr temp = lruhead;
    lruptr prev = temp;
    temp = temp->nextlruelem;
    while (temp != NULL)
    {
        if (strstr(temp->input, input) != NULL)
        {
            lruptr del = temp;
            prev->nextlruelem = del->nextlruelem;
            temp = del->nextlruelem;
            lrulen--;
            del->nextlruelem = NULL;
            free(del);
        }
        else
        {
            prev = temp;
            temp = temp->nextlruelem;
        }
    }
}
TriePtr maketrienode(int alphabet_size)
{
    TriePtr t = (TriePtr)malloc(sizeof(trienode));
    for (int i = 0; i < alphabet_size; i++)
    {
        t->children[i] = NULL;
    }
    t->flag = 0;
    t->ss_num = -1;
    return t;
}
void insertintrie(char *l, int s)
{
    char *p = (char *)calloc(strlen(l) + 1, sizeof(char));
    strcpy(p, l);
    TriePtr current = T;
    int i = 0;
    while (p[i] != '\0')
    {
        int index = (int)p[i] - 32;
        if (current->children[index] == NULL)
        {
            current->children[index] = maketrienode(95);
        }
        current = current->children[index];
        i++;
    }
    current->flag = 1;
    current->ss_num = s;
}

int searchintrie(char *l)
{
    char *p = (char *)calloc(strlen(l) + 1, sizeof(char));
    strcpy(p, l);
    int i = 0;
    if (T == NULL)
    {
        return -1;
    }
    TriePtr current = T;
    while (p[i] != '\0')
    {

        int index = (int)p[i] - 32;
        if (current->children[index] == NULL)
        {
            return -1;
        }
        else
        {
            current = current->children[index];
            i++;
        }
    }
    if (current != NULL && current->flag == 1)
    {
        return current->ss_num;
    }
    else
    {
        return -1;
    }
}

void deleteFromTrieRecursive(TriePtr t)
{
    if (t == NULL)
    {
        return;
    }

    for (int i = 0; i < 95; i++)
    {
        if (t->children[i] != NULL)
        {
            deleteFromTrieRecursive(t->children[i]);
            free(t->children[i]);
            t->children[i] = NULL;
        }
    }
}

void deleteFromTrie(char *l)
{
    char *p = (char *)calloc(strlen(l) + 1, sizeof(char));
    strcpy(p, l);
    int i = 0;
    if (T == NULL)
    {
        return;
    }
    TriePtr current = T;
    TriePtr parent = NULL;
    int parentIndex = -1;
    while (p[i] != '\0')
    {
        int index = (int)p[i] - 32;
        if (current->children[index] == NULL)
        {
            // String not found in trie
            return;
        }
        else
        {
            parent = current;
            parentIndex = index;
            current = current->children[index];
            i++;
        }
    }
    // If the string exists, mark its flag as 0 to indicate deletion
    if (current != NULL && current->flag == 1)
    {
        current->flag = 0;
    }

    // Recursively delete child nodes
    deleteFromTrieRecursive(current);

    // Remove the parent's reference to the deleted node
    if (parent != NULL)
    {
        parent->children[parentIndex] = NULL;
    }
}

ssptr create_node(int ssnum, recievedstruct recvd)
{
    ssptr newnode = (ssptr)malloc(sizeof(storage_server));
    newnode->nextss = NULL;
    newnode->avaliable = 1;
    newnode->backed_up = 0;
    newnode->r1 = 0;
    newnode->r2 = 0;
    strcpy(newnode->list_of_all_files, recvd.all_files);
    strcpy(newnode->client_port, recvd.Client_port);
    strcpy(newnode->ip_addr, recvd.ip_address);
    strcpy(newnode->nms_port, recvd.NM_port);
    newnode->sto_ser_num = ssnum;
    newnode->numb_files = recvd.num_files;
    strcpy(newnode->nms_port, recvd.NM_port);
    char back[500];
    char str[20];
    strcpy(back, "backup_");
    sprintf(str, "%d", newnode->sto_ser_num);
    strcat(back, str);
    pthread_mutex_lock(&trielock);
    insertintrie(back, ssnum);
    pthread_mutex_unlock(&trielock);
    if (newnode->sto_ser_num == 0)
    {
        return newnode;
    }
    char *delim = "$";
    char *token = strtok(recvd.all_files, delim);
    while (token != NULL)
    {
        // strcpy(newnode->list_of_all_files[j], token);
        // printf("%s\n", token);
        filesllptr ins_file = createnewfilenodell(token);
        pthread_mutex_lock(&allfileslock);
        insertfile(ins_file);
        pthread_mutex_unlock(&allfileslock);
        pthread_mutex_lock(&trielock); // change made
        insertintrie(token, newnode->sto_ser_num);
        pthread_mutex_unlock(&trielock); // change made
        // j++;
        token = strtok(NULL, delim);
    }

    return newnode;
}

void insertinssLL(ssptr insert)
{
    if (storageserverhead->nextss == NULL)
    {
        storageserverhead->nextss = insert;
    }
    else
    {
        ssptr prev = storageserverhead->nextss;
        storageserverhead->nextss = insert;
        insert->nextss = prev;
    }
}

void printssLL()
{
    if (storageserverhead->nextss == NULL)
    {
        return;
    }
    ssptr temp = storageserverhead->nextss;
    while (temp != NULL)
    {
        printf("%d %s %s %d\n", temp->sto_ser_num, temp->nms_port, temp->client_port, temp->numb_files);
        temp = temp->nextss;
    }
}

void printlru()
{
    lruptr temp = lruhead;
    temp = temp->nextlruelem;
    printf("PRINTING LRU of length %d\n", lrulen);
    while (temp != NULL)
    {
        printf("%s %s %d %s %s\n", temp->ip_address, temp->Client_port, temp->ss_num, temp->input, temp->ded_nm_port);
        temp = temp->nextlruelem;
    }
}

NS_to_client find_from_ss(char *input)
{
    NS_to_client msg;
    strcpy(msg.client_port, "invalid");
    strcpy(msg.ip_address, "invalid");
    strcpy(msg.ded_nm_port, "invalid");
    msg.ss_num = -1;
    int found = -1;
    lruptr temp = lruhead;
    lruptr prevlru = temp;
    temp = temp->nextlruelem;
    while (temp != NULL)
    {
        if (strcmp(input, temp->input) == 0)
        {
            found = 1;
            strcpy(msg.client_port, temp->Client_port);
            strcpy(msg.ip_address, temp->ip_address);
            msg.ss_num = temp->ss_num;
            strcpy(msg.ded_nm_port, temp->ded_nm_port);
            prevlru->nextlruelem = temp->nextlruelem;
            lruptr prevhead = lruhead->nextlruelem;
            temp->nextlruelem = prevhead;
            lruhead->nextlruelem = temp;
            break;
        }
        prevlru = temp;
        temp = temp->nextlruelem;
    }
    if (found == -1)
    {
        int k = searchintrie(input);
        if (k > 0)
        {
            ssptr sstemp = storageserverhead;
            found = 1;
            sstemp = sstemp->nextss;
            while (sstemp != NULL)
            {
                if (k == sstemp->sto_ser_num)
                {
                    strcpy(msg.client_port, sstemp->client_port);
                    strcpy(msg.ip_address, sstemp->ip_addr);
                    strcpy(msg.ded_nm_port, sstemp->nms_port);
                    msg.ss_num = k;
                    lruptr newnode = createlrunode(sstemp->sto_ser_num, input, msg.client_port, msg.ip_address, msg.ded_nm_port);
                    pthread_mutex_lock(&lrulock);
                    insertnewlrunode(newnode);
                    pthread_mutex_unlock(&lrulock);
                    break;
                }
                sstemp = sstemp->nextss;
            }
        }
    }
    // printlru();
    return msg;
}

void read_write(char *path, int c_client_sock, int choice)
{
    NS_to_client found = find_from_ss(path);

    if (found.ss_num == -1) // change made
    {
        // printf("sending details as %s %s %s\n", found.ip_address, found.client_port, found.ded_nm_port);
        if (send(c_client_sock, &found, sizeof(found), 0) < 0)
        {
            printf("\033[1;31mCan't sendRed \033[1;0m\n");
            return;
        }
        return;
    }

    if (choice == 1 || choice == 3)
    {
        filesllptr temp = fileshead;
        temp = temp->nxt;
        while (temp != NULL)
        {
            if (strcmp(temp->name, path) == 0)
            {
                break;
            }
            temp = temp->nxt;
        }
        pthread_mutex_lock(&temp->mutex);
        temp->reader_count++;
        if (temp->reader_count == 1)
        {
            pthread_mutex_lock(&temp->write_mutex);
        }
        pthread_mutex_unlock(&temp->mutex);
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Storage server to connect details as %s %s %s\n", found.ip_address, found.client_port, found.ded_nm_port);
        fclose(book);
        pthread_mutex_unlock(&booklock);

        if (send(c_client_sock, &found, sizeof(found), 0) < 0)
        {
            printf("\033[1;31mCan't send\n");
            return;
        }
        char feedback[256]; // change made
        int recack;
        memset(&recack, '\0', sizeof(recack));
        if (recv(c_client_sock, &recack, sizeof(recack), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 : Client Disconnected\033[1;0m\n");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 : Client Disconnected\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);
            return;
        }
        if (recack == -1)
        {
            if (choice == 1)
            {
                printf("\033[1;31mREAD FAILED DUE TO LACK OF PERMISSIONS\n\033[1;0m");
                pthread_mutex_lock(&booklock);
                book = fopen("book.txt", "a+");

                fprintf(book, "READ FAILED DUE TO LACK OF PERMISSIONS\n");
                fclose(book);
                pthread_mutex_unlock(&booklock);
                memset(feedback, '\0', sizeof(feedback));
                strcpy(feedback, "ERROR 407 :Permissisons Denied\n");
            }
            else if (choice == 3)
            {
                printf("\033[1;31mGET INFO FAILED DUE TO LACK OF PERMISSIONS\n\033[1;0m");
                pthread_mutex_lock(&booklock);
                book = fopen("book.txt", "a+");

                fprintf(book, "GET INFO FAILED DUE TO LACK OF PERMISSIONS\n");
                fclose(book);
                pthread_mutex_unlock(&booklock);
                memset(feedback, '\0', sizeof(feedback));
                strcpy(feedback, "ERROR 407 :Permissisons Denied\n");
            }
        }
        else if (recack == 1)
        {
            if (choice == 1)
            {
                printf("\033[1;32mACK : STOP Recived While Reading\n\033[1;0m");
                pthread_mutex_lock(&booklock);
                book = fopen("book.txt", "a+");

                fprintf(book, "ACK : STOP Recived While Reading\n");
                fclose(book);
                pthread_mutex_unlock(&booklock);
                memset(feedback, '\0', sizeof(feedback));
                strcpy(feedback, "FILE READ SUCCESSFULLY\n");
            }
            else if (choice == 3)
            {
                printf("\033[1;32mACK : GET INFO\n\033[1;0m");
                pthread_mutex_lock(&booklock);
                book = fopen("book.txt", "a+");

                fprintf(book, "ACK : GET INFO\n");
                fclose(book);
                pthread_mutex_unlock(&booklock);
                memset(feedback, '\0', sizeof(feedback));
                strcpy(feedback, "FILE INFO RETRIEVED SUCCESSFULLY\n");
            }
        }

        send(c_client_sock, feedback, sizeof(feedback), 0);
        pthread_mutex_lock(&temp->mutex);
        temp->reader_count--;
        if (temp->reader_count == 0)
        {
            pthread_mutex_unlock(&temp->write_mutex);
        }
        pthread_mutex_unlock(&temp->mutex);
        return;
    }
    if (choice == 2)
    {
        filesllptr tp = fileshead;
        tp = tp->nxt;
        while (tp != NULL)
        {
            if (strcmp(tp->name, path) == 0)
            {
                break;
            }
            tp = tp->nxt;
        }
        pthread_mutex_lock(&tp->write_mutex);
        if (send(c_client_sock, &found, sizeof(found), 0) < 0)
        {
            printf("Can't send\n");
            return;
        }

        int recack;
        char feedback[256]; // change made
        memset(&recack, '\0', sizeof(recack));
        if (recv(c_client_sock, &recack, sizeof(recack), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 : Client Disconnected\033[1;0m\n");
            return;
        }
        if (recack == -1)
        {

            printf("\033[1;31mWRITE FAILED DUE TO LACK OF PERMISSIONS\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "WRITE FAILED DUE TO LACK OF PERMISSIONS\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);
            memset(feedback, '\0', sizeof(feedback));
            strcpy(feedback, "ERROR 407 :Permissisons Denied\n");
        }
        else if (recack == 1)
        {

            printf("\033[1;32mACK : WRITE\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ACK : WRITE\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);
            memset(feedback, '\0', sizeof(feedback));
            strcpy(feedback, "FILE WRITTEN UPON SUCCESSFULLY\n");
        }
        send(c_client_sock, feedback, sizeof(feedback), 0);
        pthread_mutex_unlock(&tp->write_mutex);
        return; // change made
    }
}

int create_file_or_dir(char *path, int choice, int isDir) // change made
{
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Attempting Creating file or Directory\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);
    int i = strlen(path) - 1;
    while (path[i] != '/')
    {
        i--;
    }
    char src_path[1000];
    int j = 0;
    for (j = 0; j < i; j++)
    {
        src_path[j] = path[j];
    }
    src_path[j] = '\0';
    NS_to_client found = find_from_ss(src_path);
    if (found.ss_num == -1)
    {
        printf("\033[1;31mERROR 404 :No such file or folder exists\n\033[1;0m");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "No such path exists in storage\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        return -1; // change made
    }
    int ns_socket;
    struct sockaddr_in ss_address;
    // Create socket
    if ((ns_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        pthread_mutex_lock(&booklock);
        perror("Socket creation failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Socket Creation failed\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        return -1; // doubt
    }
    // printf("to be connected on %d and address %s\n", atoi(found.ded_nm_port), found.ip_address);
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Connecting on %d and address %s\n", atoi(found.ded_nm_port), found.ip_address);
    fclose(book);
    pthread_mutex_unlock(&booklock);

    // Set up server address structure
    ss_address.sin_family = AF_INET;
    ss_address.sin_addr.s_addr = inet_addr(found.ip_address);
    ss_address.sin_port = htons(atoi(found.ded_nm_port));

    // Connect to the server
    if (connect(ns_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
    {
        perror("Connection failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Connection Failed\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        return -1;
    }
    send(ns_socket, &choice, sizeof(choice), 0);
    send(ns_socket, &isDir, sizeof(isDir), 0);
    send(ns_socket, path, strlen(path), 0);

    int returned_value;
    memset(&returned_value, 0, sizeof(returned_value));
    if (recv(ns_socket, &returned_value, sizeof(returned_value), 0) <= 0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\n\033[1;0m");
        return -1;
    }
    if (returned_value == 0)
    {
        printf("ACK FROM STORAGE SERVER RECIEVED FOR CREATION\n"); // change made
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "ACK FROM STORAGE SERVER RECIEVED FOR CREATION\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        pthread_mutex_lock(&trielock);
        insertintrie(path, found.ss_num);
        pthread_mutex_unlock(&trielock);
        filesllptr new = createnewfilenodell(path);
        pthread_mutex_lock(&allfileslock);
        insertfile(new);
        pthread_mutex_unlock(&allfileslock);
    }
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Successful in Creation\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);
    close(ns_socket);
    return returned_value; // change made
}
int delete_file(char *path, int choice) // change made
{
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Attempting Deleting file or Directory\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);
    NS_to_client found = find_from_ss(path);
    if (found.ss_num == -1)
    {
        printf("\033[1;31mERROR 404 :No such file or folder exists\n\033[1;0m");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "No such file or folder exists\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        return -2; // change made
    }

    int ns_socket;
    struct sockaddr_in ss_address;
    // Create socket
    if ((ns_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // printf("to be connected on %d and address %s\n", atoi(found.ded_nm_port), found.ip_address);
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Connecting on %d and address %s\n", atoi(found.ded_nm_port), found.ip_address);
    fclose(book);
    pthread_mutex_unlock(&booklock);

    // Set up server address structure
    ss_address.sin_family = AF_INET;
    ss_address.sin_addr.s_addr = inet_addr(found.ip_address);
    ss_address.sin_port = htons(atoi(found.ded_nm_port));

    // Connect to the server
    if (connect(ns_socket, (struct sockaddr *)&ss_address, sizeof(ss_address)) == -1)
    {
        perror("Connection failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Connection to the storage server failed\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        return -1;
    }
    send(ns_socket, &choice, sizeof(choice), 0);
    send(ns_socket, path, strlen(path), 0);
    int returned_value;
    memset(&returned_value, 0, sizeof(returned_value));
    if (recv(ns_socket, &returned_value, sizeof(returned_value), 0) <= 0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\n\033[1;0m");
        return -1;
    }
    if (returned_value == 0)
    {
        printf("\033[1;32mACK FROM STORAGE SERVER RECIEVED FOR DELETION\033[1;0m\n"); // change made
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "ACK FROM STORAGE SERVER RECIEVED FOR DELETION\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        pthread_mutex_lock(&trielock);
        deleteFromTrie(path);
        pthread_mutex_unlock(&trielock);
        pthread_mutex_lock(&allfileslock);
        deleteafile(path);
        pthread_mutex_unlock(&allfileslock);
        pthread_mutex_lock(&lrulock);
        deletelrunode(path);
        pthread_mutex_unlock(&lrulock);
    }
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Successfully deleted\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);

    close(ns_socket);
    return returned_value; // change made
}

void copy_spec6(char *src_path, char *dest_path, int choice)
{
    printf("paths sent to func copy_spec6 : %s %s", src_path, dest_path);

    NS_to_client found_src;
    if (choice == 6)
    {
        found_src = find_from_ss(src_path);
        if (found_src.ss_num == -1)
        {
            printf("No such source path exists\n");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "No such source path exists\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);
        }
    }
    NS_to_client found_dest = find_from_ss(dest_path);
    if (found_dest.ss_num == -1)
    {
        printf("No such destination path exists");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");
        fprintf(book, "No such destination path exists\n");

        fclose(book);
        pthread_mutex_unlock(&booklock);
    }
    int ns_socket_src, ns_socket_dest;
    struct sockaddr_in ss_address_src, ss_address_dest;

    if (choice == 6 && strcmp(found_dest.client_port,found_src.client_port)==0)
    {
        if ((ns_socket_src = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("Socket creation failed");
            book = fopen("book.txt", "a+");
            fprintf(book, "Socket creation failed\n");
            fclose(book);
            return ;
        }
        ss_address_src.sin_family = AF_INET;
        ss_address_src.sin_addr.s_addr = inet_addr(found_src.ip_address);
        ss_address_src.sin_port = htons(atoi(found_src.ded_nm_port));
        if (connect(ns_socket_src, (struct sockaddr *)&ss_address_src, sizeof(ss_address_src)) == -1)
        {
            perror("Connection failed");
            book = fopen("book.txt", "a+");

            fprintf(book, "Connection failed\n");
            fclose(book);
            return;
        }
        int src_ss=15; // same source and destination
        printf("same source dest case\n");
        send(ns_socket_src, &src_ss, sizeof(src_ss), 0);
        char both[5000];
        memset(both,'\0',5000);
        strcpy(both,src_path);
        // strcat(both,"$");
        // strcat(both,dest_path);
        send(ns_socket_src, both, strlen(both), 0);
        recv(ns_socket_src,src_path,strlen(src_path),0);
        send(ns_socket_src, dest_path, strlen(dest_path), 0);
        printf("all sent\n");
        return;
    } 

    if ((ns_socket_src = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");
        fprintf(book, "Socket creation failed\n");

        fclose(book);
        pthread_mutex_unlock(&booklock);
        return;
    }
    // printf("to be connected on %d and address %s\n", atoi(found_src.ded_nm_port), found_src.ip_address);
    // Set up server address structure
    if (choice == 6)
    {
        ss_address_src.sin_family = AF_INET;
        ss_address_src.sin_addr.s_addr = inet_addr(found_src.ip_address);
        ss_address_src.sin_port = htons(atoi(found_src.ded_nm_port));
    }
    else
    {
        ss_address_src.sin_family = AF_INET;
        ss_address_src.sin_addr.s_addr = inet_addr(found_dest.ip_address);
        ss_address_src.sin_port = htons(atoi(src_path));
    }
    // Connect to the server
    if (connect(ns_socket_src, (struct sockaddr *)&ss_address_src, sizeof(ss_address_src)) == -1)
    {
        perror("Connection failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Connection failed\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        return;
    }

    if ((ns_socket_dest = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Socket creation failed\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        return;
    }
    // printf("to be connected on %d and address %s\n", atoi(found_dest.ded_nm_port), found_dest.ip_address);
    // Set up server address structure
    ss_address_dest.sin_family = AF_INET;
    ss_address_dest.sin_addr.s_addr = inet_addr(found_dest.ip_address);
    ss_address_dest.sin_port = htons(atoi(found_dest.ded_nm_port));

    // Connect to the server
    if (connect(ns_socket_dest, (struct sockaddr *)&ss_address_dest, sizeof(ss_address_dest)) == -1)
    {
        perror("Connection failed");
        return;
    }
    int src_ss = 12;
    int dest_ss = 13;
    int red_ss = 14;
    if (choice == 6)
    {
        send(ns_socket_src, &src_ss, sizeof(src_ss), 0);
    }
    else
    {
        send(ns_socket_src, &red_ss, sizeof(red_ss), 0);
    }

    send(ns_socket_dest, &dest_ss, sizeof(dest_ss), 0);

    send(ns_socket_src, src_path, strlen(src_path), 0);
    send(ns_socket_dest, dest_path, strlen(dest_path), 0);
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Sending Paths to Relevant Storage Server\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);

    char received_data[1005];
    while (1)
    {
        memset(received_data, '\0', sizeof(received_data));
        recv(ns_socket_src, received_data, sizeof(received_data), 0);
        if (strstr(received_data, "{.{NS_Done}.}") != NULL)
        {
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Copy Completed\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);
            char stop[100];
            strcpy(stop, "{.{.{STOP}.}.}\0");
            printf("copy done\n");
            send(ns_socket_dest, stop, strlen(stop), 0);
            break;
        }
        send(ns_socket_dest, received_data, sizeof(received_data), 0);
        memset(received_data, '\0', sizeof(received_data));
        // recv(ns_socket_dest, received_data, sizeof(received_data), 0);
        // // printf("1\n");  // future
        // send(ns_socket_src, received_data, sizeof(received_data), 0);
    }
}

void *handle_ss_connection(void *socket_desc)
{
    int communication_ss_socket = *(int *)socket_desc;

    recievedstruct recievedmessage;
    memset(&recievedmessage, 0, sizeof(recievedmessage));
    if (recv(communication_ss_socket, &recievedmessage, sizeof(recievedmessage), 0) <= 0)
    {
        printf("\033[1;31mERROR 405 : SS Disconnected\033[1;0m\n");

        return NULL;
    }

    ssptr check = storageserverhead;
    check = check->nextss;
    int found = 0;
    while (check != NULL)
    {
        if (strcmp(recievedmessage.NM_port, check->nms_port) == 0)
        {
            printf("\033[1;33mSS %d Recovered\n\033[1;33m", check->sto_ser_num);
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Storage server %d recovered\n", check->sto_ser_num);
            fclose(book);
            pthread_mutex_unlock(&booklock);

            check->avaliable = 1;
            found = 1;
            break;
        }
        check = check->nextss;
    }
    ssptr new;
    if (found == 0)
    {
        pthread_mutex_lock(&numstorlock);
        num_stor++;
        num_online_stor++;
        pthread_mutex_unlock(&numstorlock);

        recievedmessage.all_files[strlen(recievedmessage.all_files) - 1] = '\0'; // change made

        // printf("recieved message from %d is %s\n", num_stor, recievedmessage.all_files);
        new = create_node(num_stor, recievedmessage);
        pthread_mutex_lock(&sslllock);
        insertinssLL(new);
        // printssLL(storageserverhead);
        pthread_mutex_unlock(&sslllock);
        int send_num = new->sto_ser_num;
        send(communication_ss_socket, &send_num, sizeof(send_num), 0);
        printf("\033[1;33mStorage Server Initialized\033[1;0m\n");
        
    }
    if (found == 1)
    {
        new = check;
        int send_num = -1;
        send(communication_ss_socket, &send_num, sizeof(send_num), 0);
    }
    char ns_message[2000];
    memset(ns_message, '\0', sizeof(ns_message));
    strcpy(ns_message, "Storage Server Initialized");
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Storage server connected\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);

    if (send(communication_ss_socket, ns_message, strlen(ns_message), 0) < 0)
    {
        printf("Can't send\n");
        return NULL;
    }
    char ping_message[256];

    while (1)
    {
        memset(ping_message, '\0', sizeof(ping_message));
        if (recv(communication_ss_socket, ping_message, sizeof(ping_message), 0) <= 0)
        {
            printf("\033[1;37mStorage server exited\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Storage Server Exited\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            new->avaliable = 0;
            pthread_mutex_lock(&numstorlock);
            num_online_stor--;
            pthread_mutex_unlock(&numstorlock);
            return NULL;
        }
        // printf("SS %d %s\n", new->sto_ser_num, ping_message);
        fflush(stdout);
    }
}
void *handle_client_connection(void *socekt_desc)
{
    int communication_client_socket = *(int *)socekt_desc;
    // while (1)
    // {
    printf("\033[1;32m A New Client Enters\n\033[1;0m");
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "nEW CLIENT ENTERS\n");
    fclose(book);
    pthread_mutex_unlock(&booklock);
    
    int choice;
    memset(&choice, 0, sizeof(choice));
    if (recv(communication_client_socket, &choice, sizeof(choice), 0) <= 0)
    {
        printf("\033[1;31mERROR 301 :Client Disconnected\n\033[1;0m");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Client Disconeected\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        return NULL;
    }
    pthread_mutex_lock(&booklock);
    book = fopen("book.txt", "a+");

    fprintf(book, "Choice Entered : %d \n", choice);
    fclose(book);
    pthread_mutex_unlock(&booklock);

    if (choice == 1)
    {
        printf("\033[1;35mClient Requested to Read\n\033[1;0m");
        char path[1000];
        memset(path, '\0', sizeof(path));
        if (recv(communication_client_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        read_write(path, communication_client_socket, choice);
    }
    else if (choice == 2)
    {
        printf("\033[1;35mClient Requested to Write\n\033[1;0m");
        char path[1000];

        memset(path, '\0', sizeof(path));
        if (recv(communication_client_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        read_write(path, communication_client_socket, choice);
    }
    else if (choice == 3)
    {
        printf("\033[1;35mClient Requested to GET INFO\n\033[1;0m");
        char path[1000];
        memset(path, '\0', sizeof(path));
        if (recv(communication_client_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        read_write(path, communication_client_socket, choice);
    }
    else if (choice == 4)
    {
        printf("\033[1;35mClient Requested to Create\n\033[1;0m");
        char path[1000];
        int isDir;
        memset(&isDir, 0, sizeof(isDir));
        if (recv(communication_client_socket, &isDir, sizeof(isDir), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Directory : %d\n", isDir);
        fclose(book);
        pthread_mutex_unlock(&booklock);
        memset(path, '\0', sizeof(path));
        if (recv(communication_client_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        int re = create_file_or_dir(path, choice, isDir);
        char feedback[256]; // change made
        memset(feedback, '\0', sizeof(feedback));
        if (re == 0)
        {
            strcpy(feedback, "CREATION SUCCESSFUL\n");
        }
        else if (re == -1)
        {
            strcpy(feedback, "CREATION UNSUCCESSFUL\n");
        }
        send(communication_client_socket, feedback, sizeof(feedback), 0);
    }
    else if (choice == 5)
    {
        printf("\033[1;35mClient Requested to Delete\n\033[1;0m");
        char path[1000];
        memset(path, '\0', sizeof(path));
        if (recv(communication_client_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        int re = delete_file(path, choice);
        char feedback[256]; // change made
        memset(feedback, '\0', sizeof(feedback));
        if (re == 0)
        {
            strcpy(feedback, "DELETION SUCCESSFUL\n");
        }
        else if (re == -1)
        {
            strcpy(feedback, "DELETION UNSUCCESSFUL\n");
        }
        else if (re == -2)
        {
            strcpy(feedback, "No such file/directory exists\n");
        }
        send(communication_client_socket, feedback, sizeof(feedback), 0);
    }
    else if (choice == 6)
    {
        printf("\033[1;35mClient Requested to Copy\n\033[1;0m");
        printf("entered in choice==6\n");
        fflush(stdout);
        char src_path[1000];
        char dest_path[1000];
        memset(src_path, '\0', sizeof(src_path));
        if (recv(communication_client_socket, src_path, sizeof(src_path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        printf("src path is %s \n", src_path);
        memset(dest_path, '\0', sizeof(dest_path));
        if (recv(communication_client_socket, dest_path, sizeof(dest_path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 :Client Disconnected Could not proceed with its request\n\033[1;0m");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "ERROR 301 :Client Disconeected Could not proceed with its request\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            return NULL;
        }
        printf("recieved %s %s\n", src_path, dest_path);
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Paths received : %s %s\n", src_path, dest_path);
        fclose(book);
        pthread_mutex_unlock(&booklock);

        fflush(stdout);
        copy_spec6(src_path, dest_path, choice);

        char donemessage[1000];
        strcpy(donemessage, "ACK");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Acknowledgement : Copy\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        send(communication_client_socket, donemessage, strlen(donemessage), 0);
    }
    else if (choice == 7)
    {
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Exiting\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        // break;
    }
    // }
    close(communication_client_socket);
}

void *handle_redundancy(void *arg)
{
    // printf("1\n");
    while (1)
    {
        // printf("2\n");

        if (num_online_stor > 2)
        {
            printf("3\n");

            ssptr first = storageserverhead->nextss;
            ssptr last = NULL;
            ssptr temp = storageserverhead->nextss;
            printf("3_1\n");
            while (temp != NULL)
            {
                printf("3_2\n");
                if (temp->avaliable && temp->backed_up != 1)
                {
                    printf("4\n");
                    ssptr find = first;
                    while (find != NULL)
                    {
                        printf("5\n");
                        if (find->avaliable && find != temp)
                        {
                            printf("6\n");
                            char str[10];
                            temp->r1 = find->sto_ser_num; // temp ki copy in find
                            char new_redundant_folder_to_be_created_to_store_backup[100];
                            strcpy(new_redundant_folder_to_be_created_to_store_backup, "backup_");
                            sprintf(str, "%d", find->sto_ser_num);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, str);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, "/");
                            strcat(new_redundant_folder_to_be_created_to_store_backup, "Bup_");
                            sprintf(str, "%d", temp->sto_ser_num);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, str);
                            printf("%s\n", new_redundant_folder_to_be_created_to_store_backup);
                            printf("7\n");
                            int d = create_file_or_dir(new_redundant_folder_to_be_created_to_store_backup, 4, 1);
                            printf("8\n");
                            copy_spec6(temp->nms_port, new_redundant_folder_to_be_created_to_store_backup, 20);
                            pthread_mutex_lock(&booklock);
                            book = fopen("book.txt", "a+");

                            fprintf(book, "BackUp Created for %d in %d \n", temp->sto_ser_num, find->sto_ser_num);
                            fclose(book);
                            pthread_mutex_unlock(&booklock);

                            printf("9\n");
                            // red
                            find = find->nextss;
                            // all data from temp to find copy...
                            // and the new folder and the data in the trie
                            break;
                        }
                        find = find->nextss;
                    }
                    while (find != NULL)
                    {
                        if (find->avaliable && find != temp)
                        {
                            char str[10];
                            temp->r2 = find->sto_ser_num; // temp ki copy in find
                            char new_redundant_folder_to_be_created_to_store_backup[100];
                            strcpy(new_redundant_folder_to_be_created_to_store_backup, "backup_");
                            sprintf(str, "%d", find->sto_ser_num);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, str);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, "/");
                            strcat(new_redundant_folder_to_be_created_to_store_backup, "Bup_");
                            sprintf(str, "%d", temp->sto_ser_num);
                            strcat(new_redundant_folder_to_be_created_to_store_backup, str);
                            printf("%s\n", new_redundant_folder_to_be_created_to_store_backup);
                            int d = create_file_or_dir(new_redundant_folder_to_be_created_to_store_backup, 4, 1);
                            pthread_mutex_lock(&booklock);
                            book = fopen("book.txt", "a+");

                            fprintf(book, "BackUp Created for %d in %d \n", temp->sto_ser_num, find->sto_ser_num);
                            fclose(book);
                            pthread_mutex_unlock(&booklock);

                            copy_spec6(temp->nms_port, new_redundant_folder_to_be_created_to_store_backup, 20);

                            // create folder in find
                            //  all data from temp to find copy...
                            //  add the new folder and the data in the trie
                            temp->backed_up = 1;
                            break;
                        }
                        find = find->nextss;
                    }
                }

                temp = temp->nextss;
            }
        }
    }
}

void *accept_ss_connections(void *socket_desc)
{
    int all_ss_socket = *(int *)socket_desc;
    int ss_socket;
    while (1)
    {
        // Accept incoming connections for storage servers socket
        if ((ss_socket = accept(all_ss_socket, NULL, NULL)) == -1)
        {
            perror("Accepting connection for all storage servers failed");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Accepting connection for all storage servers failed\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the connection for storage servers
        pthread_t ssthread;
        if (pthread_create(&ssthread, NULL, handle_ss_connection, (void *)&ss_socket) != 0)
        {
            perror("Thread creation for ss failed");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Thread creation for ss failed\n");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Storage Server entered.\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        // Detach the thread to allow it to run independently
        pthread_detach(ssthread);
    }
}

void *accept_client_connections(void *socket_desc)
{
    int all_client_socket = *(int *)socket_desc;
    int client_socket;
    while (1)
    {
        // Accept incoming connections for clients
        if ((client_socket = accept(all_client_socket, NULL, NULL)) == -1)
        {
            perror("Accepting connection for all clients failed");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Accepting connection for all clients failed");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the connection for client request
        pthread_t clientthread;
        if (pthread_create(&clientthread, NULL, handle_client_connection, (void *)&client_socket) != 0)
        {
            perror("Thread creation for client failed");
            pthread_mutex_lock(&booklock);
            book = fopen("book.txt", "a+");

            fprintf(book, "Thread creation for client failed");
            fclose(book);
            pthread_mutex_unlock(&booklock);

            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "A New Client entered.\n");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        // Detach the thread to allow it to run independently
        pthread_detach(clientthread);
    }

    return NULL;
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Give port number for the universal server and client  as a commandline argument\n");
        return -1;
    }

    book = fopen("book.txt", "w");

    fprintf(book, "%d CommandLine Arguments are passed.\n", argc);
    fclose(book);
    int universal_ssport = atoi(argv[1]);
    int universal_clientport = atoi(argv[2]);
    int max_pending_connections = 5;

    T = NULL;
    T = maketrienode(95);
    recievedstruct dummy;
    dummy.num_files = 0;
    strcpy(dummy.Client_port, "invalid");
    strcpy(dummy.NM_port, "invalid");
    strcpy(dummy.all_files, "invalid");
    // dummy.num_files = 1; //change made
    strcpy(dummy.ip_address, "invalid");
    storageserverhead = create_node(0, dummy);
    lruhead = createlrunode(-1, "invalid", "invalid", "invalid", "invalid");
    fileshead = createnewfilenodell("invalid");

    int all_ss_socket, all_client_socket;
    struct sockaddr_in all_ss_server_address, all_client_server_address;

    // Create socket for all servers
    if ((all_ss_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation for universal servers failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure for socket of storage servers
    all_ss_server_address.sin_family = AF_INET;
    all_ss_server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    all_ss_server_address.sin_port = htons(universal_ssport);

    // Bind the socket to the specified address and port
    if (bind(all_ss_socket, (struct sockaddr *)&all_ss_server_address, sizeof(all_ss_server_address)) == -1)
    {
        perror("Binding for all servers socket failed failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Binding for all servers socket failed.\n");
        fclose(book);

        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections for all storage servers socket
    if (listen(all_ss_socket, max_pending_connections) == -1)
    {
        perror("Listening for socket type 1 failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Listening for Socket 1 failed.\n");
        fclose(book);

        exit(EXIT_FAILURE);
    }

    printf("\033[1;36mServer listening on port %d for all storage servers...\033[1;0m\n", universal_ssport);
    book = fopen("book.txt", "a+");

    fprintf(book, "Server listening on port %d for all storage servers...\n", universal_ssport);
    fclose(book);

    if ((all_client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation for all clients socket failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Socket creation for all clients socket failed");
        fclose(book);

        exit(EXIT_FAILURE);
    }

    // Set up server address structure for client socket
    all_client_server_address.sin_family = AF_INET;
    all_client_server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    all_client_server_address.sin_port = htons(universal_clientport);

    // Bind the socket to the specified address and port
    if (bind(all_client_socket, (struct sockaddr *)&all_client_server_address, sizeof(all_client_server_address)) == -1)
    {
        perror("Binding for socket of clients failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Binding for socket of clients failed");
        fclose(book);

        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections for client sockets
    if (listen(all_client_socket, max_pending_connections) == -1)
    {
        perror("Listening for clients port failed");
        book = fopen("book.txt", "a+");

        fprintf(book, "Listening for clients port failed");
        fclose(book);

        exit(EXIT_FAILURE);
    }

    printf("\033[1;36mServer listening on port %d for all clients...\033[1;0m\n", universal_clientport);
    book = fopen("book.txt", "a+");

    fprintf(book, "Server listening on port %d for all clients...\n", universal_clientport);
    fclose(book);

    pthread_t allssthread, allclientthread;
    if (pthread_create(&allssthread, NULL, accept_ss_connections, (void *)&all_ss_socket) != 0)
    {
        perror("Thread creation for ss accepting failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Thread creation for Accepting SS connections failed");
        fclose(book);
        pthread_mutex_unlock(&booklock);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&allclientthread, NULL, accept_client_connections, (void *)&all_client_socket) != 0)
    {
        perror("Thread creation for client accepting failed");
        pthread_mutex_lock(&booklock);
        book = fopen("book.txt", "a+");

        fprintf(book, "Thread creation for Accepting Client connections failed");
        fclose(book);
        pthread_mutex_unlock(&booklock);

        exit(EXIT_FAILURE);
    }
    // pthread_t redundancy_thread;
    // if (pthread_create(&redundancy_thread, NULL, handle_redundancy, NULL) != 0)
    // {
    //     perror("Thread creation for handling redundancy failed");
    //     pthread_mutex_lock(&booklock);
    //     book = fopen("book.txt", "a+");
    //     fprintf(book, "Thread creation for handling redundancy failed");

    //     fclose(book);
    //     pthread_mutex_unlock(&booklock);
    //     exit(EXIT_FAILURE);
    // }
    pthread_join(allssthread, NULL);
    pthread_join(allclientthread, NULL);
    fclose(book);
    return 0;
}
