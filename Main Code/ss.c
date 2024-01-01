// reference https://www.educative.io/answers/how-to-implement-tcp-sockets-in-c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
char current_dir[4096];
#define BUFFER_SIZE 4096
#define PATH_MAX 4096
typedef struct ss_info // struct used to send to naming server for a particular ss info
{
    char ip_address[15];
    char NM_port[10];
    char Client_port[10];
    int num_files;
    char all_files[10000];
} ss_info;
void relative_path_calc(char *c1, char *c2)
{
    int i_1 = 0;
    int i_2 = 0;
    int i_3 = 0;
    while (current_dir[i_1] == c2[i_2] && i_1 < strlen(current_dir))
    {
        i_1++;
        i_2++;
    }
    i_2++;
    for (int i = i_2; i < strlen(c2); i++)
    {
        c1[i_3++] = c2[i];
    }
    c1[i_3] = '\0';
    // printf("%s\n",c1);
}
void getLastPathComponent(const char *path, char *result) {
    const char *lastSlash = strrchr(path, '/');

    if (lastSlash != NULL) {
        strcpy(result, lastSlash + 1); // Copies the substring after the last slash
    } else {
        strcpy(result, path); // If no slash is found, copy the entire path
    }
}
void copyFile(const char *srcPath, const char *destPath)
{
    int srcFile, destFile;
    ssize_t bytesRead, bytesWritten;
    char buffer[BUFFER_SIZE];

    srcFile = open(srcPath, O_RDONLY);
    if (srcFile == -1)
    {
        perror("Error opening source file");
        return;
    }

    destFile = open(destPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destFile == -1)
    {
        perror("Error creating or opening destination file");
        close(srcFile);
        return;
    }

    while ((bytesRead = read(srcFile, buffer, BUFFER_SIZE)) > 0)
    {
        bytesWritten = write(destFile, buffer, bytesRead);
        if (bytesWritten != bytesRead)
        {
            perror("Error writing to destination file");
            close(srcFile);
            close(destFile);
            return;
        }
    }

    close(srcFile);
    close(destFile);
}

void copyDirectory(const char *srcDir, const char *destDir)
{
    printf("in the f\n");
    DIR *dir;
    struct dirent *entry;
    struct stat statBuf;
    char srcPath[PATH_MAX];
    char destPath[PATH_MAX];

    if (lstat(srcDir, &statBuf) == -1)
    {
        perror("Error getting file status");
        return;
    }

    if (S_ISREG(statBuf.st_mode))
    { // If source path is a file
        copyFile(srcDir, destDir);
        return;
    }

    if ((dir = opendir(srcDir)) == NULL)
    {
        perror("Error opening source directory");
        return;
    }

    // if (mkdir(destDir, 0755) == -1 && errno != EEXIST) {
    mkdir(destDir, 0755);
    //     perror("Error creating destination directory");
    //     closedir(dir);
    //     return;
    // }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(srcPath, PATH_MAX, "%s/%s", srcDir, entry->d_name);
        snprintf(destPath, PATH_MAX, "%s/%s", destDir, entry->d_name);

        if (lstat(srcPath, &statBuf) == -1)
        {
            perror("Error getting file status");
            continue;
        }

        if (S_ISDIR(statBuf.st_mode))
        {
            copyDirectory(srcPath, destPath);
        }
        else if (S_ISREG(statBuf.st_mode))
        {
            copyFile(srcPath, destPath);
        }
    }

    closedir(dir);
}
void peekkar(char *path)
{
    struct dirent *dir_entry;
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    while ((dir_entry = readdir(dir)) != NULL)
    {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
        {
            continue; // Skip "." and ".."
        }

        char full_path[4097];
        char to_be_concat[4097];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);

        struct stat file_details;
        if (stat(full_path, &file_details) == -1)
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(file_details.st_mode))
        {
            // *count = *count + 1;
            relative_path_calc(to_be_concat, full_path);
            // strcat(all_files, to_be_concat);
            if (to_be_concat[0] == '.')
            {
                continue;
            }
            printf("%s\n", to_be_concat);
            // strcat(all_files, "$");
            peekkar(full_path);
        }
        else if (file_details.st_mode & S_IXUSR)
        {
            // *count = *count + 1;
            relative_path_calc(to_be_concat, full_path);
            if (to_be_concat[0] == '.')
            {
                continue;
            }
            // strcat(all_files, to_be_concat);
            printf("%s\n", to_be_concat);

            // strcat(all_files, "$");
        }
        else
        {
            // *count = *count + 1;
            relative_path_calc(to_be_concat, full_path);
            if (to_be_concat[0] == '.')
            {
                continue;
            }
            // strcat(all_files, to_be_concat);
            printf("%s\n", to_be_concat);

            // strcat(all_files, "$");
        }
    }

    closedir(dir);
}

void sendFileData(const char *filePath, int client_socket)
{

    int file_desc = open(filePath, O_RDONLY);

    if (file_desc == -1)
    {
        perror("Error opening file");
        printf("%s\n", filePath);
        return;
    }

    int chunk_size = 1e3;
    char *arr = (char *)malloc(sizeof(char) * chunk_size );
    memset(arr, '\0', chunk_size );

    long long int file_size = lseek(file_desc, 0, SEEK_END);
    lseek(file_desc, 0, SEEK_SET);
    long long int count = 0;

    while (count < file_size)
    {
        // usleep(100);
        if (file_size - count < chunk_size-1)
        {
            memset(arr, '\0', chunk_size);
            int len = read(file_desc, arr, file_size - count);
            send(client_socket, arr, strlen(arr), 0);
            memset(arr, '\0', chunk_size);
            count = file_size;
        }
        else
        {
            memset(arr, '\0', chunk_size);
            int len = read(file_desc, arr, chunk_size-1);
            send(client_socket, arr, chunk_size, 0);
            memset(arr, '\0', chunk_size);
            count += chunk_size-1;
        }
        // recv(client_socket,arr,chunk_size,0);  //ack-kushal
    }

    char ack[100];
    strcpy(ack, "{.{.{ST");
    strcat(ack, "OP}.}.}\0");
    send(client_socket, ack, strlen(ack), 0);
    free(arr);
    // Close the file
    close(file_desc);
}

void relative_path_calc_copy(char *c1, char *c2, char *current_dir)
{
    int i_1 = 0;
    int i_2 = 0;
    int i_3 = 0;
    while (current_dir[i_1] == c2[i_2] && i_1 < strlen(current_dir))
    {
        i_1++;
        i_2++;
    }
    i_2++;
    for (int i = i_2; i < strlen(c2); i++)
    {
        c1[i_3++] = c2[i];
    }
    c1[i_3] = '\0';
    // printf("%s\n",c1);
}
void copy_peekkar(char *path, char all_files[10000], int *count, char *curr_dir, int new_socket)
{
    struct dirent *dir_entry;
    DIR *dir = opendir(path);

    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    while ((dir_entry = readdir(dir)) != NULL)
    {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0)
        {
            continue; // Skip "." and ".."
        }

        char full_path[4097];
        char to_be_concat[4097];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dir_entry->d_name);
        char curr_file[5000];
        struct stat file_details;
        if (stat(full_path, &file_details) == -1)
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(file_details.st_mode))
        {
            // printf("%s\n",full_path);
            *count = *count + 1;
            relative_path_calc_copy(to_be_concat, full_path, curr_dir);
            strcat(all_files, "{.{D}.}");
            strcat(all_files, to_be_concat);
            strcat(all_files, "$");

            strcpy(curr_file, "{.{D}.}");
            strcat(curr_file, to_be_concat);
            // printf("%s\n", curr_file);
            // send(new_socket, curr_file, strlen(curr_file), 0);

            copy_peekkar(full_path, all_files, count, curr_dir, new_socket);
        }
        else if (file_details.st_mode & S_IXUSR)
        {
            *count = *count + 1;
            relative_path_calc_copy(to_be_concat, full_path, curr_dir);
            strcat(all_files, "{.{F}.}");
            strcat(all_files, to_be_concat);

            strcat(all_files, "$");

            strcpy(curr_file, "{.{F}.}");
            strcat(curr_file, to_be_concat);
            // printf("%s\n", curr_file);
            // send(new_socket, curr_file, strlen(curr_file), 0);

            // printf("%s\n", to_be_concat);
            // printf("%s\n",full_path);
        }
        else
        {
            *count = *count + 1;
            relative_path_calc_copy(to_be_concat, full_path, curr_dir);
            strcat(all_files, "{.{F}.}");
            strcat(all_files, to_be_concat);
            // printf("%s\n", to_be_concat);

            strcat(all_files, "$");

            strcpy(curr_file, "{.{F}.}");
            strcat(curr_file, to_be_concat);
            // printf("%s\n", curr_file);
            // send(new_socket, curr_file, strlen(curr_file), 0);

            // printf("%s\n",full_path);
        }
    }
    // printf("%s\n",all_files);

    closedir(dir);
}

void SEND_DATA(char *currentDirectory, char *filePaths, int client_socket)
{
    // char filePaths[5000] = "{.{F}.}s.c${.{D}.}check_1${.{D}.}check_1/check_3${.{F}.}check_1/check_3/f_5.c${.{F}.}check_1/f_1.c${.{F}.}check_1/f_2.c${.{D}.}check_2${.{F}.}check_2/f_4.c${.{F}.}s${.{F}.}a.out${.{D}.}client${.{F}.}client/a.out${.{F}.}client/c.c$";
    char *pathCopy = strdup(filePaths);
    char *token = strtok(pathCopy, "$");

    // char* token =(char*)calloc(MAX_BUFFER_SIZE,sizeof(char));
    // int stop_received = 0;

    while (token != NULL)
    {
        // memset(token, 0, MAX_BUFFER_SIZE);
        // recv(client_socket, token, MAX_BUFFER_SIZE, 0);

        printf("Received: %s\n", token);

        // Check for directory or file indicator at the beginning of the path
        if (strncmp(token, "{.{D}.}", 7) == 0)
        {
            // Directory path
            const char *directoryPath = token + 7; // Skip the indicator "{.{D}.}"
        }
        else if (strncmp(token, "{.{F}.}", 7) == 0)
        {
            // File path
            const char *filePath = token + 7; // Skip the indicator "{.{F}.}"

            char currentPath[5000] = "";
            snprintf(currentPath, sizeof(currentPath), "%s/%s", currentDirectory, filePath);

            printf("Opening/Creating file: %s\n", currentPath);
            int rec_ack_create = 5;

            // int res = recv(client_socket, &rec_ack_create, sizeof(rec_ack_create), 0);

            // if (res == 1)
            // {
            printf("file created, now sending data \n");
            // sendFileData(filePath, client_socket);
            sendFileData(currentPath, client_socket);
            // }
            // else if (res == 0)
            // {
            //     printf("couldn't open file so not sending data\n");
            // }

            // int cont=0;
            // sleep(0.01);
            // send(client_socket,&cont,sizeof(cont),0);
            // printf("%d\n",cont);
        }

        token = strtok(NULL, "$");
    }

    free(pathCopy);
    free(token);
}
void fileordir(char *path,char all_files[10000], int *count, char *curr_dir, int new_socket,char* g_c_d)
{
    
    struct stat path_stat;
    if (stat(path, &path_stat) == 0) {
        if (S_ISREG(path_stat.st_mode)) {
            char to_be_concat[500];
            memset(to_be_concat,'\0',500);
            // printf("%s is a regular file.\n", path);
            // relative_path_calc_copy(to_be_concat, full_path, curr_dir);
            getLastPathComponent(path,to_be_concat);
            strcpy(all_files, "{.{F}.}");
            strcat(all_files, to_be_concat);
            // strcat(all_files, '\0');


            strcat(all_files, "$\0");
            printf("%s\n", all_files);
            send(new_socket, all_files, strlen(all_files), 0);
            char dummy[1000];
        // recv(ns_communication_socket,dummy,1000,0); //future
            sendFileData(path, new_socket);



        } else if (S_ISDIR(path_stat.st_mode)) {
            // printf("%s is a directory.\n", path);
            copy_peekkar(path, all_files, count, curr_dir, new_socket);
            send(new_socket, all_files, strlen(all_files), 0);
            // char dummy[1000];
            // recv(ns_communication_socket,dummy,1000,0); //future
            SEND_DATA(path, all_files, new_socket);
            // SEND_DATA(g_c_d, all_files, new_socket);
        } else {
            printf("%s is neither a file nor a directory.\n", path);
        }
    } else {
        perror("Error");
    }

}
void createFilesAndDirectories(const char *currentDirectory, char *filePaths, int client_socket)
{
    // char filePaths[5000] = "{.{F}.}s.c${.{D}.}check_1${.{D}.}check_1/check_3${.{F}.}check_1/check_3/f_5.c${.{F}.}check_1/f_1.c${.{F}.}check_1/f_2.c${.{D}.}check_2${.{F}.}check_2/f_4.c${.{F}.}s${.{F}.}a.out${.{D}.}client${.{F}.}client/a.out${.{F}.}client/c.c$";
    char *pathCopy = strdup(filePaths);
    char *token = strtok(pathCopy, "$");

    // char* token =(char*)calloc(MAX_BUFFER_SIZE,sizeof(char));
    // int stop_received = 0;

    while (token != NULL)
    {
        // memset(token, 0, MAX_BUFFER_SIZE);
        // recv(client_socket, token, MAX_BUFFER_SIZE, 0);

        printf("Received: %s\n", token);

        // Check for directory or file indicator at the beginning of the path
        if (strncmp(token, "{.{D}.}", 7) == 0)
        {
            // Directory path
            const char *directoryPath = token + 7; // Skip the indicator "{.{D}.}"
            char right_path[1000];

            strcpy(right_path, currentDirectory);
            strcat(right_path, "/");
            strcat(right_path, directoryPath);
            printf("Creating directory: %s\n", right_path);
            if (mkdir(right_path, 0777) == -1)
            {
                perror("Error creating directory");
            }

            memset(right_path, '\0', 1000);
        }
        else if (strncmp(token, "{.{F}.}", 7) == 0)
        {
            // File path
            const char *filePath = token + 7; // Skip the indicator "{.{F}.}"

            char currentPath[5000] = "";
            snprintf(currentPath, sizeof(currentPath), "%s/%s", currentDirectory, filePath);
            int failed = 0;
            int success = 1;
            int res = 5;
            printf("Opening/Creating file: %s\n", currentPath);
            FILE *file = fopen(currentPath, "a+");
            if (file == NULL)
            {
                perror("Error opening/creating file");
                // res = send(client_socket, &failed, sizeof(failed), 0);
                // printf("%d\n", res);
            }
            else
            {
                fclose(file); // Close the file
                // res= send(client_socket,&success,sizeof(success),0);
                // printf("%d\n",res);
                //    char *data_received = (char *)malloc(sizeof(char) * 1e5);
                //     while (1) {
                //         int len = recv(client_socket, data_received, 1e5, 0);

                //         if (len <= 0) {
                //             // Handle the case when the received data length is zero or less (indicating the end of transmission)
                //             break;
                //         }

                //         if (strstr(data_received, "{.{.{STOP}.}.}") != NULL) {
                //             break; // STOP signal received
                //         }

                //         // Write the received data into the file
                //         size_t bytes_written = fwrite(data_received, 1, len, file);

                //         if (bytes_written != len) {
                //             // Handle error writing data to file
                //             perror("Error writing data to file");
                //             break;
                //         }

                //         memset(data_received, '\0', 1e5);
                //     }

                //     fclose(file);
                //     sleep(0.01);
                //     // int cont=1;
                //     // recv(client_socket,&cont,sizeof(cont),0);
                //     free(data_received);

                char *data_recieved = (char *)malloc(sizeof(char) * 1e3 );

                while (1)
                {
                    memset(data_recieved, '\0', 1e3 );
                    int len = recv(client_socket, data_recieved, 1e3-1, 0);

                    if (len <= 0)
                    {
                        // printf("Error receiving data\n");
                        break;
                    }

                    data_recieved[len] = '\0'; // Null-terminate the received data
                    // printf("%s\n",data_recieved);
                    char *stop_ptr = strstr(data_recieved, "{.{.{STOP}.}.}");
                    if (stop_ptr != NULL)
                    {
                        // Calculate the position of the stop string in the received data
                        int stop_position = stop_ptr - data_recieved;

                        // Open the file for writing
                        FILE *file = fopen(currentPath, "a+");
                        if (file == NULL)
                        {
                            perror("Error opening file");
                            break;
                        }

                        // Write the portion of data received before the STOP string
                        fwrite(data_recieved, sizeof(char), stop_position, file);
                        // send(client_socket,data_recieved,strlen(data_recieved),0); future

                        fclose(file); // Close the file

                        break; // STOP signal received
                    }

                    // Open the file for writing
                    FILE *file = fopen(currentPath, "a+");
                    if (file == NULL)
                    {
                        perror("Error opening file");
                        break;
                    }

                    // Write the entire data received to the file if STOP string is not found yet
                    fwrite(data_recieved, sizeof(char), len, file);
                    // send(client_socket,data_recieved,strlen(data_recieved),0);  future

                    fclose(file); // Close the file
                }
            }
            // memset(token, 0, MAX_BUFFER_SIZE);
        }

        token = strtok(NULL, "$");
    }
    memset(token,'\0',5000);
    free(pathCopy);
    free(token);
    // printf("copying done\n");
}

void read_file_ss(char *filePath, int client_id)
{
    int k;
    int file_desc = open(filePath, O_RDONLY);

    if (file_desc == -1)
    {
        printf("\033[1;31mERROR 407 : Permissions Denied\033[1;0m\n");
        memset(&k, 0, sizeof(k));
        k = -1;
        send(client_id, &k, sizeof(k), 0);
        return;
        // Return NULL to indicate an error
    }
    memset(&k, 0, sizeof(k));
    k = 1;
    send(client_id, &k, sizeof(k), 0);
    int chunk_size = 1e3;
    char arr[chunk_size];
    long long int file_size = lseek(file_desc, 0, SEEK_END);
    lseek(file_desc, 0, SEEK_SET);
    long long int count = 0;
    // printf("starting sending the data\n");
    while (count < file_size)
    {

        if (file_size - count < chunk_size)
        {
            memset(arr, '\0', strlen(arr));
            int len = read(file_desc, arr, file_size - count);
            if (len == -1)
            {
                perror("Read error");
            }
            arr[len] = '\0';
            int k = send(client_id, arr, len, 0);
            if (k < 0)
            {
                perror("Send Error\n");
            }
            memset(arr, '\0', strlen(arr));
            count = file_size;
        }
        else
        {
            memset(arr, '\0', strlen(arr));

            int len = read(file_desc, arr, chunk_size);
            if (len == -1)
            {
                perror("Read error");
            }
            arr[len] = '\0';
            // printf("%d %s", len, arr); //change made
            int k = send(client_id, arr, len, 0);
            if (k < 0)
            {
                perror("Send Error\n");
            }
            memset(arr, '\0', strlen(arr));
            count = count + chunk_size;
        }
    }
    char ack[100];
    strcpy(ack, "STOP");
    ack[strlen(ack)] = '\0';
    send(client_id, ack, strlen(ack), 0);

    // Close the file
    close(file_desc);
}

void write_to_file(char *filename, int client_id)
{
    int k;
    char path[1000];
    int i = 0;
    while (filename[i] != ';')
    {
        path[i] = filename[i];
        i++;
    }
    path[i++] = '\0';

    // printf("%s\n", path);
    // i++;
    FILE *file_desc = fopen(path, "w+");

    if (file_desc == NULL)
    {
        printf("\033[1;31mError 407 : Permissions Denied\033[1;0m\n");
        memset(&k, 0, sizeof(k));
        k = -1;
        send(client_id, &k, sizeof(k), 0);
        return;
    }
    memset(&k, 0, sizeof(k));
    k = 1;
    send(client_id, &k, sizeof(k), 0);

    int chunk_size = 1e3;

    // char *arr = (char *)malloc(sizeof(char) * chunk_size);
    char arr[chunk_size];
    int c = 0;
    while (filename[i] != '\0')
    {
        arr[c++] = filename[i++];
    }
    arr[c++] = '\0';

    // printf("%s\n", arr);
    // printf("%ld\n", strlen(arr));
    int w = fwrite(arr, sizeof(char), strlen(arr), file_desc);
    // int w = fprintf(file_desc, arr, chunk_size);

    // Write the content to the file
    // fprintf(file, "%s", content);

    // Close the file
    fclose(file_desc);

    return;
    // printf("Content has been successfully overwritten to '%s'\n", filename);
}

void get_info(char *path, int client_id)
{

    struct stat file_stat;
    int k;
    if (stat(path, &file_stat) == -1)
    {
        printf("\033[1;31mERROR 407 : Permissions Denied\033[1;0m\n");
        memset(&k, 0, sizeof(k));
        k = -1;
        send(client_id, &k, sizeof(k), 0);
        return;
    }
    memset(&k, 0, sizeof(k));
    k = 1;
    send(client_id, &k, sizeof(k), 0);
    char permissions[1000];

    if ((file_stat.st_mode & S_IRUSR) != 0)
    {
        permissions[0] = 'r';
    }
    else
    {
        permissions[0] = '-';
    }

    if ((file_stat.st_mode & S_IWUSR) != 0)
    {
        permissions[1] = 'w';
    }
    else
    {
        permissions[1] = '-';
    }

    if ((file_stat.st_mode & S_IXUSR) != 0)
    {
        permissions[2] = 'x';
    }
    else
    {
        permissions[2] = '-';
    }
    permissions[3] = ' ';
    if ((file_stat.st_mode & S_IRGRP) != 0)
    {
        permissions[4] = 'r';
    }
    else
    {
        permissions[4] = '-';
    }

    if ((file_stat.st_mode & S_IWGRP) != 0)
    {
        permissions[5] = 'w';
    }
    else
    {
        permissions[5] = '-';
    }

    if ((file_stat.st_mode & S_IXGRP) != 0)
    {
        permissions[6] = 'x';
    }
    else
    {
        permissions[6] = '-';
    }
    permissions[7] = ' ';
    if ((file_stat.st_mode & S_IROTH) != 0)
    {
        permissions[8] = 'r';
    }
    else
    {
        permissions[8] = '-';
    }

    if ((file_stat.st_mode & S_IWOTH) != 0)
    {
        permissions[9] = 'w';
    }
    else
    {
        permissions[9] = '-';
    }
    if ((file_stat.st_mode & S_IXOTH) != 0)
    {
        permissions[10] = 'x';
    }
    else
    {

        permissions[10] = '-';
    }
    send(client_id, permissions, sizeof(permissions), 0);
    long long int file_sz = (long long)file_stat.st_size;
    send(client_id, &file_sz, sizeof(long long int), 0);
    return;
}

int createFileOrDirectory(char *path, int isDirectory)
{

    if (isDirectory)
    {
        // Create a directory with the given path and name
        if (mkdir(path, 0777) == -1)
        {
            printf("\033[1;31mError creating directory\n\033[1;0m");
            return -1;
        }
        else
        {
            printf("\033[1;32mDirectory '%s' created successfully.\033[1;0m\n", path);
            return 0;
        }
    }
    else
    {
        // Create a file with the given path and name
        FILE *file = fopen(path, "w");
        if (file == NULL)
        {
            printf("\033[1;31mError creating file\n\033[1;0m");
            return -1;
        }
        else
        {
            fclose(file);
            printf("\033[1;32mFile '%s' created successfully.\n\033[1;0m", path);
            return 0;
        }
    }
}

int deleteFileOrDirectory(char *path)
{
    // Check if the file or directory exists
    if (access(path, F_OK) != 0)
    {
        printf("\033[1;31mERROR 404 : File or directory '%s' does not exist.\n\033[1;0m", path);
        return -1;
    }

    char command[256];
    snprintf(command, sizeof(command), "rm -r %s", path);

    // Execute the shell command
    int result = system(command);

    if (result == 0)
    {
        printf("\033[1;32mDirectory '%s' removed successfully.\033[1;0m\n", path);
        return 0;
    }
    else
    {
        printf("\033[1;31m Error removing directory\033[1;0m");
        return -1;
    }
}

void *handle_ns_request(void *socket_desc)
{
    int ns_communication_socket = *(int *)socket_desc;
    int choice;
    memset(&choice, 0, sizeof(choice));
    if (recv(ns_communication_socket, &choice, sizeof(choice), 0) <= 0)
    {
        printf("\033[1;31mERROR 406 : NS Disconnected\n\033[1;0m");
        return NULL;
    }
    if (choice == 4)
    {
        printf("\033[1;33mClient requested to Create via NS\n\033[1;0m");
        int isDir;
        memset(&isDir, 0, sizeof(isDir));
        if (recv(ns_communication_socket, &isDir, sizeof(isDir), 0) <= 0)
        {
            printf("\033[1;31mERROR 406 : NS Disconnected\n\033[1;0m");
            return NULL;
        }
        char path[1000];
        memset(path, '\0', 1000);
        if (recv(ns_communication_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 406 : NS Disconnected\n\033[1;0m");
            return NULL;
        }
        int k = createFileOrDirectory(path, isDir);
        send(ns_communication_socket, &k, sizeof(k), 0);
    }
    else if (choice == 5)
    {
        printf("\033[1;33mClient requested to Delete via NS\n\033[1;0m");
        char path[1000];
        memset(path, '\0', 1000);
        if (recv(ns_communication_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 406 : NS Disconnected\n\033[1;0m");
            return NULL;
        }
        int k = deleteFileOrDirectory(path);
        send(ns_communication_socket, &k, sizeof(k), 0);
    }
    else if (choice == 12)
    {
        // char src_path[1000];
        // memset(src_path, '\0', sizeof(src_path));
        // if (recv(ns_communication_socket, src_path, sizeof(src_path), 0) <= 0)
        // {
        //     printf("ERROR 406 :NS Disconnected\n");
        //     return NULL;
        // }
        // printf("path recieved at source SS is %s\n", src_path);
        // char all_files[10000];
        // int count = 0;
        // char curr_dir[2000];
        // getcwd(curr_dir, 2000);
        // char comp_src_path[2500];
        // strcpy(comp_src_path, curr_dir);
        // strcat(comp_src_path, "/");
        // strcat(comp_src_path, src_path);
        // printf("path passed - %s\n", comp_src_path);
        // // strcat(all_files,"{.{D}.}");
        // // strcat(all_files,);

        // copy_peekkar(comp_src_path, all_files, &count, comp_src_path, ns_communication_socket);
        // send(ns_communication_socket, all_files, strlen(all_files), 0);
        // char dummy[1000];
        // // recv(ns_communication_socket,dummy,1000,0); //future
        // SEND_DATA(comp_src_path, all_files, ns_communication_socket);
        // char done[100];
        // strcpy(done, "{.{NS_");
        // strcat(done, "Done}.}\0");

        // send(ns_communication_socket, done, strlen(done), 0);

        // printf("all_files sent to the NMS.\n");

        char src_path[1000];
        int bytes;
        memset(src_path, '\0', sizeof(src_path));
        if (recv(ns_communication_socket, src_path, sizeof(src_path), 0) < 0)
        {
            printf("Couldn't receive\n");
            return NULL;
        }

        printf("path recieved at source SS is %s\n", src_path);
        char all_files[10000];
        int count = 0;
        char curr_dir[2000];
        getcwd(curr_dir, 2000);
        char comp_src_path[2500];
        strcpy(comp_src_path, curr_dir);
        strcat(comp_src_path, "/");
        strcat(comp_src_path, src_path);
        printf("path passed - %s\n", comp_src_path);
        // strcat(all_files,"{.{D}.}");
        // strcat(all_files,);

        fileordir(comp_src_path, all_files, &count, comp_src_path, ns_communication_socket,curr_dir);
        // send(ns_communication_socket, all_files, strlen(all_files), 0);
        // char dummy[1000];
        // // recv(ns_communication_socket,dummy,1000,0); //future
        // SEND_DATA(curr_dir, all_files, ns_communication_socket);
        // SEND_DATA(comp_src_path, all_files, ns_communication_socket);
        char done[100];
        strcpy(done, "{.{NS_");
        strcat(done, "Done}.}\0");

        send(ns_communication_socket, done, strlen(done), 0);

        printf("all_files sent to the NMS.\n");
    }
    else if (choice == 13)
    {
        char dest_path[1000];
        memset(dest_path, '\0', sizeof(dest_path));
        if (recv(ns_communication_socket, dest_path, sizeof(dest_path), 0) < 0)
        {
            printf("Couldn't receive\n");
            return NULL;
        }
        printf("path recieved at dest SS is %s\n", dest_path);
        char token[5000];
        int MAX_BUFFER_SIZE = 1e3;
        memset(token, '\0', sizeof(token));
        recv(ns_communication_socket, token, MAX_BUFFER_SIZE, 0);

        // send(ns_communication_socket, token, strlen(all_files), 0); future

        char curr_dir[2000];
        // getcwd(curr_dir,2000);
        // strcat(curr_dir,"/");
        strcat(curr_dir, dest_path);
        printf("path passed - %s\n", curr_dir);
        createFilesAndDirectories(curr_dir, token, ns_communication_socket);
    }
    else if (choice == 14)
    {
        char src_path[1000];
        memset(src_path, '\0', sizeof(src_path));
        if (recv(ns_communication_socket, src_path, sizeof(src_path), 0) < 0)
        {
            printf("Couldn't receive\n");
            return NULL;
        }
        printf("path recieved at source SS is %s\n", src_path);
        char all_files[10000];
        int count = 0;
        char curr_dir[2000];
        getcwd(curr_dir, 2000);
        char comp_src_path[2500];
        strcpy(comp_src_path, curr_dir);
        // strcat(comp_src_path, "/");
        // strcat(comp_src_path, src_path);
        printf("path passed - %s\n", comp_src_path);
        // strcat(all_files,"{.{D}.}");
        // strcat(all_files,);

        copy_peekkar(comp_src_path, all_files, &count, comp_src_path, ns_communication_socket);
        send(ns_communication_socket, all_files, strlen(all_files), 0);
        char dummy[1000];
        // recv(ns_communication_socket,dummy,1000,0); //future
        SEND_DATA(comp_src_path, all_files, ns_communication_socket);
        char done[100];
        strcpy(done, "{.{NS_");
        strcat(done, "Done}.}\0");

        send(ns_communication_socket, done, strlen(done), 0);

        printf("all_files sent to the NMS.\n");
    }
    else if(choice=15)
    {
        char src_path[1000];
        memset(src_path, '\0', sizeof(src_path));
        if (recv(ns_communication_socket, src_path, sizeof(src_path), 0) < 0)
        {
            printf("Couldn't receive\n");
            return NULL;
        }
        // printf("path recieved at source SS is %s\n", src_path);
        send(ns_communication_socket,src_path,strlen(src_path),0);
        char dest_path[1000];
        memset(dest_path, '\0', sizeof(dest_path));
        if (recv(ns_communication_socket, dest_path, sizeof(dest_path), 0) < 0)
        {
            printf("Couldn't receive\n");
            return NULL;
        }
        // printf("path recieved at dest SS is %s\n", dest_path);
        copyDirectory(src_path,dest_path);
        // printf("GG\n");

    }
    // Close the client socket
    close(ns_communication_socket);
}
void *handle_client_request(void *socket_desc)
{
    printf("\033[1;35mClient Connected\n\033[1;0m");
    int client_communication_socket = *(int *)socket_desc;
    int choice;
    memset(&choice, 0, sizeof(choice));
    if (recv(client_communication_socket, &choice, sizeof(choice), 0) <= 0)
    {
        printf("\033[1;31mERROR 301 : Client Disconnected\n\033[1;0m");
        return NULL;
    }
    if (choice == 1)
    {
        printf("\033[1;36mClient Requested to Read\033[1;0m\n");
        char path[1000];
        memset(path, '\0', 1000);
        if (recv(client_communication_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 : Client Disconnected\n\033[1;0m");
            return NULL;
        }
        read_file_ss(path, client_communication_socket);
    }
    else if (choice == 2)
    {
        printf("\033[1;36mClient Requested to Write\033[1;0m\n");
        char packet[1000];
        memset(packet, '\0', 1000);

        if (recv(client_communication_socket, packet, sizeof(packet), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 : Client Disconnected\n\033[1;0m");
            return NULL;
        }
        write_to_file(packet, client_communication_socket);
    }
    else if (choice == 3)
    {
        printf("\033[1;36mClient Requested to GET INFO\033[1;0m\n");
        char path[1000];
        memset(path, '\0', 1000);

        if (recv(client_communication_socket, path, sizeof(path), 0) <= 0)
        {
            printf("\033[1;31mERROR 301 : Client Disconnected\n\033[1;0m");
            return NULL;
        }

        get_info(path, client_communication_socket);
    }

    // Close the client socket
    close(client_communication_socket);

    pthread_exit(NULL);
}

void *accept_NS_connections(void *socket_desc)
{
    int ded_ns_socket = *(int *)socket_desc;
    int ns_accept_socket;
    while (1)
    {
        // Accept incoming connections for ded_ns socket type
        if ((ns_accept_socket = accept(ded_ns_socket, NULL, NULL)) == -1)
        {
            perror("Accepting connection for ded_ns failed");
            exit(EXIT_FAILURE);
        }
        printf("NS arrived here\n");
        // Create a new thread to handle the connection for NS
        pthread_t nsrequest;
        if (pthread_create(&nsrequest, NULL, handle_ns_request, (void *)&ns_accept_socket) != 0)
        {
            perror("Thread creation for handling request of NS failed");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to allow it to run independently
        pthread_detach(nsrequest);
    }
    return NULL;
}

void *accept_client_connections(void *socket_desc)
{
    int client_socket = *(int *)socket_desc;
    int client_accept_socket;

    while (1)
    {
        // Accept incoming connections for clients
        if ((client_accept_socket = accept(client_socket, NULL, NULL)) == -1)
        {
            perror("Accepting connection for client failed");
            exit(EXIT_FAILURE);
        }

        // Create a new thread to handle the connection for clients
        pthread_t clientrequest;
        if (pthread_create(&clientrequest, NULL, handle_client_request, (void *)&client_accept_socket) != 0)
        {
            perror("Thread creation for hadnling client failed");
            exit(EXIT_FAILURE);
        }

        // Detach the thread to allow it to run independently
        pthread_detach(clientrequest);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Give 3 ports as command line arguments universal NM port,Dedicated NS port, Dedicated client port\n");
        return -1;
    }

    int ded_ns_socket, client_socket;
    struct sockaddr_in ded_ns_address, client_address;
    if ((ded_ns_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("\033[1;31mSocket creation for Dedicated NS port failed\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure for ded_ns socket
    ded_ns_address.sin_family = AF_INET;
    ded_ns_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    ded_ns_address.sin_port = htons(atoi(argv[2]));

    // Bind the socket to the specified address and port
    if (bind(ded_ns_socket, (struct sockaddr *)&ded_ns_address, sizeof(ded_ns_address)) == -1)
    {
        printf("\033[1;31mBinding for Dedicated NS socket failed Use some other port\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections for ded_ns socket
    if (listen(ded_ns_socket, 5) == -1)
    {
        printf("\033[1;31mListening for ded_ns socket failed\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    printf("\033[1;33mServer listening on port %d for ded_ns...\033[1;0m\n", atoi(argv[2]));

    // Create socket for clients socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("\033[1;31mSocket creation for client socket failed\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure for client socket
    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    client_address.sin_port = htons(atoi(argv[3]));

    // Bind the socket to the specified address and port
    if (bind(client_socket, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
    {
        printf("\033[1;31mBinding for client socket failed use some other port\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections for client socket
    if (listen(client_socket, 5) == -1)
    {
        printf("\033[1;31mListening for client socket failed\n\033[1;0m");
        exit(EXIT_FAILURE);
    }

    printf("\033[1;33mServer listening on port %d for clients...\033[1;0m\n", atoi(argv[3]));

    pthread_t NSrequestthread, clientsthread;

    if (pthread_create(&NSrequestthread, NULL, accept_NS_connections, (void *)&ded_ns_socket) != 0)
    {
        printf("\033[1;31mThread creation for accepting NS requests failed\n\033[1;0m");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&clientsthread, NULL, accept_client_connections, (void *)&client_socket) != 0)
    {
        printf("\033[1;31mThread creation for accepting client request failed\n\033[1;0m");
        exit(EXIT_FAILURE);
    }

    // Initialization of SS code (SS as a client for NS at universal port)
    int ns_universal_socket_desc;
    struct sockaddr_in ns_universal_addr;
    char ns_universal_message[2000];
    int ns_universal_port = atoi(argv[1]);
    // Clean buffers:
    memset(ns_universal_message, '\0', sizeof(ns_universal_message));

    // Create socket:
    ns_universal_socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (ns_universal_socket_desc < 0)
    {
        printf("\033[1;31mUnable to create socket\033[1;0m\n");
        exit(EXIT_FAILURE);
    }

    // Set port and IP the same as server-side:
    ns_universal_addr.sin_family = AF_INET;
    ns_universal_addr.sin_port = htons(ns_universal_port);
    ns_universal_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Send connection request to server:
    if (connect(ns_universal_socket_desc, (struct sockaddr *)&ns_universal_addr, sizeof(ns_universal_addr)) < 0)
    {
        printf("\033[1;31mUnable to connect\033[1;0m\n");
        exit(EXIT_FAILURE);
    }
    printf("\033[1;34mConnected with Naming Server successfully\033[1;0m\n");
    memset(ns_universal_message, '\0', sizeof(ns_universal_message));

    char curr[4097];
    if (getcwd(curr, sizeof(curr)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    if (getcwd(current_dir, sizeof(current_dir)) == NULL)
    {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    ss_info initialize;
    strcpy(initialize.ip_address, "127.0.0.1");
    strcpy(initialize.NM_port, argv[2]);
    strcpy(initialize.Client_port, argv[3]);
    strcpy(initialize.all_files, "\0");
    initialize.num_files = 0;
    printf("\033[1;36mFiles stored by this storage servers are : \n\033[1;37m");
    peekkar(curr);
    char commandlineinput[513];
    int slash_n_count = 0;
    printf("\033[1;33mEnter all the paths that should be accessible[Do not enter ./a.out and backup folder]\033[1;0m\n");
    while (1)
    {
        if (fgets(commandlineinput, sizeof(commandlineinput), stdin) == NULL)
        {
            break;
        }
        if (commandlineinput[0] == '\n' && slash_n_count == 0)
        {
            slash_n_count++;
            continue;
        }
        else if (commandlineinput[0] == '\n' && slash_n_count == 1)
        {
            break;
        }

        if (commandlineinput[strlen(commandlineinput) - 1] == '\n')
        {
            commandlineinput[strlen(commandlineinput) - 1] = '\0';
        }
        strcat(initialize.all_files, commandlineinput);
        strcat(initialize.all_files, "$");
        initialize.num_files++;
    }

    // Send the message to server:
    if (send(ns_universal_socket_desc, &initialize, sizeof(initialize), 0) < 0)
    {
        printf("Unable to send message\n");
        return -1;
    }

    int ss_numrec;
    if (recv(ns_universal_socket_desc, &ss_numrec, sizeof(ss_numrec), 0) <= 0)
    {
        printf("\033[1;31mError 405 : NS disconnected\n\033[1;31m");
        exit(EXIT_FAILURE);
    }
    memset(ns_universal_message, '\0', sizeof(ns_universal_message));
    // Receive the server's response:
    if (recv(ns_universal_socket_desc, ns_universal_message, sizeof(ns_universal_message), 0) <= 0)
    {
        printf("\033[1;31mError 405 : NS disconnected\n\033[1;31m");
        exit(EXIT_FAILURE);
    }
    printf("\033[1;33m%s\033[1;0m\n", ns_universal_message);
    // memset(ns_universal_message, '\0', sizeof(ns_universal_message));

    // printf("waiting for next message\n");
    // if (recv(ns_universal_socket_desc, ns_universal_message, sizeof(ns_universal_message), 0) < 0)
    // {
    //     printf("Error while receiving server's msg\n");
    //     return -1;
    // }
    // Close the socket:
    if (ss_numrec != -1)
    {
        char str[20];
        char req_str[1000];
        strcpy(req_str, "backup_");
        sprintf(str, "%d", ss_numrec);
        strcat(req_str, str);
        // printf("%s\n", req_str);
        mkdir(req_str, 0777);
    }
    char ping_message[256];
    struct timespec start, end;
    while (1)
    {
        sleep(3);
        memset(ping_message, '\0', 0);
        strcpy(ping_message, "Alive");
        // printf("Sending\n");
        send(ns_universal_socket_desc, ping_message, sizeof(ping_message), 0);
    }
    pthread_join(NSrequestthread, NULL);
    pthread_join(clientsthread, NULL);
    close(ns_universal_socket_desc);
    close(ded_ns_socket);
    close(client_socket);
    // printf("%d\n",initialize.num_files);

    return 0;
}