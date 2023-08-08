#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

#include "queue.h"
#include "hash.h"

#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4

static FILE *logfile;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_t *thread_list_pointer;
int threadz = DEFAULT_THREAD_COUNT;
Queue *q;
int running = 1;
HashTable *ht;

#define LOG(...) fprintf(logfile, __VA_ARGS__);

// Converts a string to an 16 bits unsigned integer.
// Returns 0 if the string is malformed or out of the range.
static size_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

#define SIZE 4096
void print(int code, int connfd, off_t size, int flag) {
    int msgsize = 0;
    char msg[100];
    char *num_string = (char *) calloc(100, sizeof(char *));
    switch (code) {
    case 200:
        write(connfd, "HTTP/1.1 200 OK\r\n", 17);
        if (!flag) {
            msgsize = 3;
            strncpy(msg, "OK\n", 3);
        } else {
            msgsize = size;
        }
        break;
    case 201:
        write(connfd, "HTTP/1.1 201 Created\r\n", 22);
        msgsize = 8;
        strncpy(msg, "Created\n", 8);
        break;
    case 400:
        write(connfd, "HTTP/1.1 400 Bad Request\r\n", 26);
        msgsize = 12;
        strncpy(msg, "Bad Request\n", 12);
        break;
    case 403:
        write(connfd, "HTTP/1.1 403 Forbidden\r\n", 24);
        msgsize = 10;
        strncpy(msg, "Forbidden\n", 10);
        break;
    case 404:
        write(connfd, "HTTP/1.1 404 Not Found\r\n", 24);
        msgsize = 10;
        strncpy(msg, "Not Found\n", 10);
        break;
    case 500:
        write(connfd, "HTTP/1.1 500 Internal Service Error\r\n", 37);
        msgsize = 23;
        strncpy(msg, "Internal Service Error\n", 23);
        break;
    case 501:
        write(connfd, "HTTP/1.1 501 Not Implemented\r\n", 30);
        msgsize = 16;
        strncpy(msg, "Not Implemented\n", 16);
    }
    int num = msgsize;
    int digits = 0;
    while (num > 0) {
        digits++;
        num /= 10;
    }
    snprintf(num_string, digits + 6, "%d \r\n\r\n", msgsize);
    write(connfd, "Content-Length:\t", 16);
    int end = 0;
    for (int i = 0; i < 100; i++) {
        if (num_string[i] == 0) {
            end = i;
            break;
        }
    }
    write(connfd, num_string, end);
    if (flag == 0) {
        write(connfd, msg, msgsize);
    }
    free(num_string);
}

int copy_file(int op, int fp, char *uri) {
    int realfp;
    int code = 200;
    char buf[BUF_SIZE];
    if (op == 1) {
        return -1;
    } else if (op == 2) {
        realfp = open(uri, O_WRONLY | O_TRUNC);
        if (realfp == -1) {
            code = 201;
            realfp = open(uri, O_WRONLY | O_CREAT, S_IRWXU);
        }
    } else {
        realfp = open(uri, O_WRONLY | O_APPEND);
        if (realfp == -1) {
            code = 404;
            return code;
        }
    }
    int bytez = 0;
    while ((bytez = read(fp, buf, BUF_SIZE)) > 0) {
        write(realfp, buf, bytez);
    }
    close(realfp);
    return code;
}

int get(char *uri, int connfd) {
    int fp;
    int code = 200;
    int bytes = 0;
    off_t size = 0;
    struct stat st;
    fp = open(uri, O_RDONLY);
    char buf[SIZE];
    if (fp == -1) {
        code = 404;
        print(code, connfd, size, 0);
    } else {
        stat(uri, &st);
        size = st.st_size;
        print(code, connfd, size, 1);
        while ((bytes = read(fp, buf, SIZE)) > 0) {
            write(connfd, buf, bytes);
        }
    }
    close(fp);
    return code;
}

int put(int fp, char *body, int size) {
    int code = 200;
    if (fp == -1) {
        code = 201;
    }
    write(fp, body, size);
    return code;
}

int append(int fp, char *body, int size) {
    int code = 200;
    if (fp == -1) {
        code = 404;
    }
    write(fp, body, size);
    return code;
}

void proc_header_string(char *header, int *length,
    int *id) { //actually need to write code to parse through this bc this is beyond dumb
    char *num_string = calloc(100, sizeof(char));
    char *key = calloc(1000, sizeof(char));
    char *id_string = calloc(100, sizeof(char));
    int keyindex = 0;
    int numindex = 0;
    int idindex = 0;
    for (
        int i = 0; i < 100;
        i++) { //reads until a colon, if what is on left side of colon is Content-Length or Request-Id then read the value
        key[keyindex] = header[i];
        keyindex++;
        if (header[i] == ':') {
            key[keyindex] = 0;
            i++;
            if (strcmp(key, "Content-Length:") == 0) {
                if (num_string[i] != 0) {
                    memset(num_string, 0, 100);
                }
                for (int j = i; j < 100; j++) {
                    num_string[numindex] = header[j];
                    numindex++;
                    if (header[j] == 0 || header[j] == '\r' || header[j] == '\n') {
                        memset(key, 0, 1000);
                        keyindex = 0;
                        break;
                    }
                }
                numindex = 0;
            } else if (strcmp(key, "Request-Id:") == 0) {
                if (id_string[i] != 0) {
                    memset(id_string, 0, 100);
                }
                for (int j = i; j < 100; j++) {
                    id_string[idindex] = header[j];
                    idindex++;
                    if (header[j] == 0 || header[j] == '\r' || header[j] == '\n') {
                        memset(key, 0, 1000);
                        keyindex = 0;
                        break;
                    }
                }
                idindex = 0;
            } else {
                memset(key, 0, 1000);
                keyindex = 0;
            }
        } else if (header[i] == 0 || header[i] == '\r' || header[i] == '\n') {
            memset(key, 0, 1000);
            keyindex = 0;
        }
    }
    //convert numstring to number, feed to *length
    *length = atoi(num_string);
    for (int i = 0; i < 100; i++) {
        if (i == 0 && num_string[i] == 0) {
            *length = 0;
        }
        if (num_string[i] == 0) {
            break;
        }
    }
    *id = atoi(id_string);
    free(num_string);
    free(id_string);
    free(key);
}

void audit_log(int op, char *uri, int code, int id) {
    char *method = NULL;
    if (op == 1) {
        method = "GET";
    } else if (op == 2) {
        method = "PUT";
    } else if (op == 3) {
        method = "APPEND";
    }
    LOG("%s,/%s,%d,%d\n", method, uri, code, id);
    fflush(logfile);
    return;
}

void proc_request_string(char *request, int *op, char **uri, int *valid_version) {
    char *method = (char *) calloc(9, sizeof(char));
    char *version = (char *) calloc(100, sizeof(char));
    int index = 0;
    for (int i = 0; i < 9; i++) {
        if (request[index] == 32) { //stop reading operation when a space is reached.
            method[i] = 0;
            break;
        }
        method[i] = request[index];
        index++;
    }
    if (strcmp(method, "GET") == 0) {
        *op = 1;
    } else if (strcmp(method, "PUT") == 0) {
        *op = 2;
    } else if (strcmp(method, "APPEND") == 0) {
        *op = 3;
    } else {
        *op = -1;
    }
    index++; //skip the space
    index++; //skip the /
    for (int i = 0; i < 100; i++) {
        if (request[index] == 32) {
            (*uri)[i] = 0;
            break;
        }
        (*uri)[i] = request[index];
        index++;
    }
    index++;
    for (int i = 0; i < 20; i++) {
        if (request[index] == 32
            || request[index] == '\r') { //when a space or \r is reached, stop reading
            version[i] = 0;
            break;
        }
        version[i] = request[index];
        index++;
    }
    if (strcmp(version, "HTTP/1.1") == 0) {
        *valid_version = 1;
    }
    free(method);
    free(version);
}

void handle_connection(int connfd) {
    // make the compiler not complain
    char buffer[BUF_SIZE];
    ssize_t bytez = 0;
    int flag = 0; //at 0 for request line, 1 for header field, 2 for message body
    int isrn
        = 0; //set to 1 if \r is found, if \n is found while this is 1 then move on to next section.
    int tworns = 0; //used to check for 2 \r\n in a row for header field
    char request[2048];
    int reqind = 0;
    char header[2048];
    int headind = 0;
    char body[BUF_SIZE];
    memset(body, 0, sizeof(body));
    int bodyind = 0;
    int bufind = 0;
    int not_first_pass = 0;
    int bytes_left = 0;
    int op = -1;
    int version = 0;
    char *uri = (char *) calloc(100, sizeof(char));
    int code = 501;
    int id = 0;
    int tmpfile = 0;
    char tmpfilename[7] = { 'X', 'X', 'X', 'X', 'X', 'X', '\0' };
    while ((bytez = read(connfd, buffer, BUF_SIZE)) > 0) {
        bufind = 0;
        if (flag == 0) { //parsing request
            for (int i = bufind; i < bytez; i++) {
                if (buffer[i] == '\n') { //when reaching \r\n
                    if (isrn == 1) {
                        flag = 1;
                        bufind++;
                        tworns = 1;
                        proc_request_string(request, &op, &uri, &version);
                        if (ht_lookup(ht, uri) == NULL) {
                            pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
                            ht_insert(ht, uri, lock);
                        }
                        break;
                    }
                } else {
                    isrn = 0;
                }
                if (buffer[i] == '\r') {
                    isrn = 1;
                }
                request[reqind] = buffer[i];
                reqind++;
                bufind++;
            }
        }
        isrn = 0;
        if (flag == 1) { //parsing headers
            for (int i = bufind; i < bytez; i++) {
                if (buffer[i] == '\n') { //iterate until you see \r\n
                    if (isrn == 1) { //if you reach \r\n
                        isrn = 0;
                        if (tworns) { //if you reach \r\n\r\n
                            flag = 2;
                            bufind++;
                            proc_header_string(header, &bytes_left, &id);
                            if (op != 1) {
                                tmpfile = mkstemp(tmpfilename);
                            }
                            break;
                        }
                        tworns = 1;
                    }
                } else if (buffer[i] == '\r') {
                    isrn = 1;
                } else {
                    isrn = 0;
                    tworns = 0;
                }
                header[headind] = buffer[i];
                headind++;
                bufind++;
            }
        }
        if (flag == 2) { //when parsing body
            for (int i = bufind; i < bytez;
                 i++) { //read until you read Content-Length number of bytes
                if (bytes_left > 0) {
                    body[bodyind] = buffer[i];
                    bodyind++;
                    bytes_left--;
                    if (bytes_left == 0) {
                        break;
                    }
                }
            }
        }
        if (flag
            == 2) { //if parsing body, write what is in the body to the file (or simply fetch it if it is GET)
            request[reqind] = 0;
            header[headind] = 0;
            if ((version == 0 || bytes_left < 0) && op != 1) {
                print(400, connfd, 0, 0);
            } else if (op == 1) {
                pthread_mutex_lock(&(ht_lookup(ht, uri)->lock));
                code = get(uri, connfd);
                pthread_mutex_unlock(&(ht_lookup(ht, uri)->lock));
            } else if (op == 2 && not_first_pass == 0) {
                code = put(tmpfile, body, bodyind);
            } else if (
                op == 3
                || not_first_pass
                       == 1) { //if doing a PUT op with length greater than size of buffer, use append when reading more bytes in
                if (not_first_pass) {
                    append(tmpfile, body, bodyind);
                } else {
                    code = append(tmpfile, body, bodyind);
                }
            }
            if (op != 1 && bytes_left == 0) {
                print(code, connfd, 0, 0);
            }
            if (bytes_left == 0) {
                pthread_mutex_lock(&(ht_lookup(ht, uri)->lock));
                close(tmpfile);
                tmpfile = open(tmpfilename, O_RDONLY);
                if (op != 1) {
                    code = copy_file(op, tmpfile, uri);
                }
                audit_log(op, uri, code, id);
                close(connfd);
                close(tmpfile);
                remove(tmpfilename);
                Node *n = ht_lookup(ht, uri);
                pthread_mutex_unlock(&(n->lock));
                //free(uri);
                return;
            }

            not_first_pass = 1;
        }
        bodyind = 0;
    }
    free(uri);
    (void) connfd;
}

void *thread_func(void *x) {
    Queue *q = (Queue *) x;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    while (running) {
        pthread_mutex_lock(&lock);
        while (queue_empty(q) && running) {
            pthread_cond_wait(&ready, &lock);
        }
        if (!running) {
            pthread_mutex_unlock(&lock);
            return NULL;
        }
        Request *x = queue_dequeue(q);
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);
        handle_connection(getconnfd(x));
        //pthread_mutex_unlock(&lock);
    }
    return NULL;
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        warnx("received SIGTERM");
        fclose(logfile);
        pthread_cond_broadcast(&ready);
        running = 0;
        for (int i = 0; i < threadz; i++) {
            pthread_cond_broadcast(&ready);
            pthread_join(thread_list_pointer[i], NULL);
        }
        queue_destroy(&q);
        ht_delete(&ht);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;
    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    //signal(SIGINT, sigterm_handler);
    signal(SIGTERM, sigterm_handler);

    int listenfd = create_listen_socket(port);
    //LOG("port=%" PRIu16 ", threads=%d\n", port, threads);
    threadz = threads;
    pthread_t thread_list[threads];
    memset(thread_list, 0, sizeof(thread_list));
    thread_list_pointer = thread_list;
    q = queue_create();
    ht = ht_create(128, false);
    for (int i = 0; i < threads; i++) {
        int err = pthread_create(&(thread_list[i]), NULL, thread_func, q);
        if (err) {
            printf("error in creating thread\n");
        }
    }
    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        pthread_mutex_lock(&lock);
        while (queue_full(q)) {
            pthread_cond_wait(&empty, &lock);
        }
        queue_enqueue(q, connfd);
        pthread_cond_signal(&ready);
        pthread_mutex_unlock(&lock);
    }

    return EXIT_SUCCESS;
}
