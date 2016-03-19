/*
 * A test webserver. Use at your own risk.
 * @Author Damiene A. Stewart
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_BUFF_SIZE 20*1024*1024

/*
 * Struct to hold server configuration information.
 */
struct config_t {
  char *root_dir;
  char *default_html;
  char *ip_address;
  int port;
  char *logfile;
} configuration;

/*
 * Structure for holding request data.
 */
struct request_t {
  char *URI;
  char *METHOD;
  char *VERSION;
  char *HOST;
  char *ACCEPT;
  char *ACCEPT_LANGUAGE;
  char *CONNECTION;
  char *ACCEPT_ENCODING;
  char *USER_AGENT;
  int CONTENT_LENGTH;
  char *CONTENT_TYPE;
  char *BODY;
};

/*
 * Read the server configuration file and load the struct param with info.
 */
void read_server_configuration(struct config_t *);

/*
 * Handle http connections.
 */
void *handle_connection(void *);

/*
 * Get the request information. If the request method was a post,
 * memory may be allocated by the function that needs to be freed
 * by the caller.
 */
void get_request_information(struct request_t *, char *);

/*
 * Print out an error message along with the error code.
 */
void error(const char *);

/*
 * Driver code.
 */
int main(int argc, char *argv[]) {
  // Variables.
  int ssocket_fd, csocket_fd;
  struct sockaddr_in server, client;

  // Read configuration file for configuration information;
  read_server_configuration(&configuration);

  // Create server socket.
  ssocket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (ssocket_fd < 0) {
    error("Failed to open socket");
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(configuration.ip_address);
  server.sin_port = htons(configuration.port);
  server.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket on the server.
  if (bind(ssocket_fd, (struct sockaddr *) &server, sizeof(server)) < 0)
  {
    error("Failed to bind to port");
  }

  if (listen(ssocket_fd, 3) < 0)
  {
      error("Failed to listen on socket");
  }

  // DEBUG
  printf("IP address: %s.\n", configuration.ip_address);
  printf("Port: %d.\n", configuration.port);
  printf("Default html document: %s.\n", configuration.default_html);
  printf("Root directory: %s.\n", configuration.root_dir);

  // Accept connections, implement threading.
  socklen_t c = sizeof(struct sockaddr_in);
  pthread_t thread_id;
  while((csocket_fd = accept(ssocket_fd, (struct sockaddr *)&client, (socklen_t*)&c))) {

    int *lcsocket_fd = malloc(sizeof(int));
    *lcsocket_fd = csocket_fd;
    if (pthread_create(&thread_id, NULL, handle_connection, lcsocket_fd)) {
      error("Issue creating thread.");
    }
    pthread_detach(thread_id);
  }

  if (csocket_fd < 0) {
    error("Accept failed.");
  }

  return 0;
}

/*
 * Read the configuration file.
 * struct config_t configuration is the configuration struct
 * that holds values from the configuration file.
 */
void read_server_configuration(struct config_t *configuration) {
  // Open the configuration file.
  FILE *config_file = fopen("../configuration/config.conf", "r");

  // Is the configuration file present?
  if (!config_file) {
    error("Configuration file not present.");
  }

  // Get the lines from the server configuration file.
  ssize_t read;
  size_t len = 0;
  int line_number = 0;
  char *line = NULL;
  char *key = NULL;
  char *value = NULL;
  int i;
  while ((read = getline(&line, &len, config_file)) != -1) {
    ++line_number;
    if ( !(key = strtok(line, " ")) || !(value = strtok(NULL, "\n")) || (line = NULL)) {
      printf("Invalid fomatting for configuration file at line: %d.", line_number);
      exit(1);
    }

    for (i = 0; i < strlen(key); i++) {
      key[i] = tolower(key[i]);
    }

    if (strcmp(key, "root_dir") == 0) {
      configuration->root_dir = value;
    } else if (strcmp(key, "default_html") == 0) {
      configuration->default_html = value;
    } else if (strcmp(key, "ip_address") == 0) {
      configuration->ip_address = value;
    } else if (strcmp(key, "port") == 0) {
      configuration->port = atoi(value);
    }
  }

  free(line);
  fclose(config_file);
}

/*
 * Handle the connection.
 */
void *handle_connection(void *csocket_fd) {
  int socket = *((int *)csocket_fd);
  char request[MAX_BUFF_SIZE];

  int read_count = 0;
  if ((read_count = read(socket, request, 4095)) < 0) {
    printf("ERROR reading from socket: %d", socket);
    return NULL;
  }

  // Create request struct.
  struct request_t *request_data = calloc(1, sizeof(struct request_t));
  get_request_information(request_data, request);

  // DEBUG INFORMATION
  printf("\n\n**REQUEST DATA**\n\n");
  printf("http version: %s\n", request_data->VERSION);
  printf("uri: %s\n", request_data->URI);
  printf("method: %s\n", request_data->METHOD);
  printf("host: %s\n", request_data->HOST);
  printf("accept: %s\n", request_data->ACCEPT);
  printf("accept-language: %s\n", request_data->ACCEPT_LANGUAGE);
  printf("accept-encoding: %s\n", request_data->ACCEPT_ENCODING);
  printf("connection: %s\n", request_data->CONNECTION);
  printf("content-type: %s\n", request_data->CONTENT_TYPE);
  printf("content-length: %d\n", request_data->CONTENT_LENGTH);
  printf("body: %s\n", request_data->BODY);


  // Directly handle request here.
  char *request_uri = request_data->URI;
  char *root_dir = configuration.root_dir;
  char *default_html = configuration.default_html;
  char *uri = NULL;

  char *choice = !(strcmp(request_uri, "/")) ? default_html : ++request_uri;

  size_t uri_s = 0;
  uri_s = strlen(root_dir) + strlen(choice) + 1; // +1 for /, +1 for \0
  uri = malloc(sizeof(char) * uri_s);
  bzero(uri, uri_s);

  memcpy(uri, root_dir, strlen(root_dir));
  uri[strlen(root_dir)] = '/';
  memcpy(uri + strlen(root_dir) + 1, choice, strlen(choice));

  printf("URI: %s\n", uri);

  if (!strcmp(request_data->METHOD, "GET")) {
    FILE *page = fopen(uri, "r");

    if (page) {
      fseek(page, 0, SEEK_END);
      long length = ftell(page);
      fseek(page, 0, SEEK_SET);

      char *page_data = malloc(length * sizeof(char) + 1); // sizeof unnecessary
      if (page_data) {
        fread(page_data, 1, length, page);
        page_data[length] = '\n';

        char *content_length_buffer = malloc(256);
        bzero(content_length_buffer, 256);
        sprintf(content_length_buffer, "Content-length: %lu\n", length);

        write(socket, "HTTP/1.1 200 OK\n", 16);
        write(socket, content_length_buffer, strlen(content_length_buffer));
        write(socket, "Content-Type: text/html\n\n", 25);
        write(socket, page_data, length+1);

      } else {

        error("Could not load data from file.");

      }

      fclose(page);

    } else {

      write(socket, "HTTP/1.1 404 Not Found\n", 24);
      write(socket, "Content-length: %d\n", 20);
      write(socket, "Content-Type: text/html\n\n", 25);
      write(socket, "<html><body><h1>Page not found.</h1></body></html>\n",51);
    }

  } else {

    write(socket, "HTTP/1.1 405 Method Not Allowed\n", 32);
    write(socket, "Content-length: %d\n", 20);
    write(socket, "Content-Type: text/html\n\n", 25);
    write(socket, "<html><body><h1>Sorry, the server does not support this method \
    yet.</h1></body></html>\n",87);

  }

  if (shutdown(socket, 2) == -1) {
    char message[256];
    snprintf(message, 255, "Problem stopping client socket: %d.\n", socket);
    error(message);
  } else {
    printf("Shutting down socket %d.\n\n", socket);
  }

  free(csocket_fd);
  return NULL;
}

/*
 * Populate request structure with request data.
 */
void get_request_information(struct request_t *request, char *message) {
  request->METHOD = strtok(message, " ");
  request->URI = strtok(NULL, " ");
  request->VERSION = strtok(NULL, "\r\n");

  // READ the rest of the request.
  message = request->VERSION + strlen(request->VERSION) + 2;

  char *header;
  char *value;
  char *tspace;
  int i;
  while (strpbrk(message, ": ")) {
    header = strtok(message, ": ");
    value = strtok(NULL, "\r\n");

    // printf("\n%s | %s\n", header, value);

    for (i = 0; i < strlen(header); i++)
      header[i] = tolower(header[i]);

    // Avoid whitespace
    for (i = 0; isspace(value[i]); i++) {
      value++;
    }

    if (!strcmp(header, "host")) {
      request->HOST = value;
    } else if (!strcmp(header, "accept")) {
      request->ACCEPT = value;
    } else if (!strcmp(header, "accept-language")) {
      request->ACCEPT_LANGUAGE = value;
    } else if (!strcmp(header, "accept-encoding")) {
      request->ACCEPT_ENCODING = value;
    } else if (!strcmp(header, "connection")) {
      request->CONNECTION = value;
    } else if (!strcmp(header, "content-length")) {
      request->CONTENT_LENGTH = atoi(value);
    } else if (!strcmp(header, "content-type")) {
      request->CONTENT_TYPE = value;
    }

    message = value + strlen(value) + 2;
  }

  // Get content if present:
  if (request->CONTENT_LENGTH > 0) {
    request->BODY = strtok(NULL, "\r\n");
    for (i = 0; isspace(value[i]); i++) {
      request->BODY++;
    }
  }
}

/*
 * Print out error message.
 */
void error(const char *theMessage) {
  perror(theMessage);
  exit(1);
}
