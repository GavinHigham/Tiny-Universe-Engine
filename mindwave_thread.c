#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <jansson.h>

#define BUFLEN 256
#define NUM_CONNECTION_ATTEMPTS 3

int mindwave_attention = 0;
int mindwave_meditation = 0;
extern int quit;

/*

alarm(10); // set a 10 second timeout
(len = connect(mysocket, &sock_dest, sizeof(struct sockaddr))) < 0 ||
(len = read(mysocket, buffer, 10));
alarm(0); // cancel alarm if it hasn't happened yet
if (len == -1 && errno == EINTR)
    // timed out before any data read
else if (len == -1)
    // other error
*/


sig_atomic_t alarm_counter;

void alarm_handler(int signal) {
    alarm_counter++;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void setup_alarm_handler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = alarm_handler;
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, 0) < 0)
        error("Can't establish signal handler");
}

int mindwave_thread(void *data)
{
    //Setup json objects
    json_error_t *errors = NULL;
    json_t *auth      = json_object();
    json_t *cfg       = json_object();
    json_t *braindata = json_object();
    json_object_set(auth, "appName", json_string("TestFour"));
    json_object_set(auth, "appKey", json_string("cad1aa3e1f6157363dd4b121d35ecdacb3a56fdf"));
    json_object_set(cfg, "enableRawOutput", json_false());
    json_object_set(cfg, "format", json_string("Json"));
    setup_alarm_handler();

    char buffer[BUFLEN];
    int sockfd, portno, n, connected = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    portno = 13854;

    for (int i = 0; i < NUM_CONNECTION_ATTEMPTS && !connected; i++) {
        printf("Connection attempt %i...\n", i+1);
        //Network stuff I don't understand. I only accept that it works. For now.
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
        server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
            error("ERROR connecting");

        char  *cfg_string = json_dumps(cfg, 0);
        char *auth_string = json_dumps(auth, 0);
        //Authenticate!
        printf("Attempting to authenticate with:\n%s\n", auth_string);
        n = write(sockfd, auth_string, strlen(auth_string));
        free(auth_string);
        if (n < 0)
            error("ERROR writing to socket");
        //Configure!
        printf("Attempting to configure with:\n%s\n", cfg_string);
        n = write(sockfd, cfg_string, strlen(cfg_string));
        free(cfg_string);
        if (n < 0)
            error("ERROR writing to socket");
        printf("Advancing to first \\r.\n");
        bzero(buffer, BUFLEN);

        alarm(5); //5 second timeout
        printf("Attempting to read one character...\n");
        int len = read(sockfd,buffer,1);
        printf("Read a character!\n");
        alarm(0); //Cancel alarm if read returns within the timeout period.
        if (len == -1) {
            error("ERROR connection closed.");
            close(sockfd);
            continue;
        }
        connected = 1;
        printf("%c", buffer[0]); //Print the garbage at the beginning, for debug
        do {
            read(sockfd,buffer,1);
            printf("%c", buffer[0]); //Print the garbage at the beginning, for debug
        } while (buffer[0] != '\r');
    }
    if (connected) {
        printf("Beginning to parse complete packets.\n");
        int k = 0;
        while (!quit)
        {
            bzero(buffer, BUFLEN);
            while(read(sockfd,&buffer[k],1) && buffer[k] != '\r' && k < BUFLEN) {
                //printf("%c", buffer[k]);
                k++;
            }
            braindata = json_loads(buffer, k, errors);
            json_t *eSense = json_object_get(braindata, "eSense");
            if (eSense != NULL) {
                int attention_tmp = (int)json_integer_value(json_object_get(eSense, "attention"));
                if (attention_tmp != 0) mindwave_attention = attention_tmp;
                int meditation_tmp = (int)json_integer_value(json_object_get(eSense, "meditation"));
                if (meditation_tmp != 0) mindwave_meditation = meditation_tmp;
                char *braindata_str = json_dumps(braindata, 0);
                printf("Attention: %i, Meditation: %i\n", mindwave_attention, mindwave_meditation);
                printf("\n%s\n\n", braindata_str);
                free(braindata_str);
            }
            k = 0;
        }
        close(sockfd);
    } else {
        printf("ERROR too many timeouts, aborting.\n");
    }
    return 0;
}