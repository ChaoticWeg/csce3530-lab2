#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OUTFILE_NAME "server.out"

#include "src/common.h"

int mock_open(int, FILE *);
int mock_close(int, FILE *);

int main(int argc, char **argv)
{
    srand((uint32_t) time(NULL));

    if (argc < 2 || (strcasecmp(argv[1], "open") != 0 && strcasecmp(argv[1], "close") != 0))
    {
        fprintf(stderr, "Usage:\n    %s open  %s\n    %s close %s\n", argv[0], HELP_OPEN, argv[0], HELP_CLOSE);
        if (argc == 1) return 1;
        fprintf(stderr, "\nInvalid argument: %s\n", argv[1]);
        return 1;
    }

    errno = 0;

    // open output file
    FILE *outfile = fopen(OUTFILE_NAME, "w");
    if (outfile == NULL || errno != 0)
        return abort_with_errno(errno, "fopen");

    printf("writing output to %s as well as console\n", OUTFILE_NAME);

    // create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1 || errno != 0)
        return abort_with_errno(errno, "socket");

    // allow address reuse if process is killed
    int32_t yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1 || errno != 0)
        return abort_with_errno(errno, "setsockopt");

    // assign port
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    // attempt to bind
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0 || errno != 0)
        return abort_with_errno(errno, "bind");

    // attempt to listen
    if (listen(sockfd, 5) != 0 || errno != 0)
        return abort_with_errno(errno, "listen");

    // nice
    printf("listening on port %d\n", ntohs(server_addr.sin_port));

    // attempt to accept
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    bzero(&client_addr, sizeof(client_addr));
    int clientfd = accept(sockfd, (struct sockaddr *) &client_addr, &client_len);
    if (clientfd == -1 || errno != 0)
        return abort_with_errno(errno, "accept");

    // nice
    printf("client connected from %s\n", inet_ntoa(client_addr.sin_addr));

    // branch
    int result = 1;

    // branch to open if opening
    if (strcasecmp(argv[1], "open") == 0)
        result = mock_open(clientfd, outfile);

    else if (strcasecmp(argv[1], "close") == 0)
        result = mock_close(clientfd, outfile);

    shutdown(clientfd, SHUT_RDWR);
    shutdown(sockfd, SHUT_RDWR);

    close(clientfd);
    close(sockfd);

    return result;
}

int mock_open(int client_fd, FILE *outfile)
{
    printf("simulating opening a TCP connection\n\n");
    errno = 0;

    mytcp_t segment;
    ssize_t rw_result;

    /* RECEIVE CONNECTION REQUEST SEGMENT */

    printf("awaiting connection request ... ");

    // attempt to receive an example tcp segment
    bzero(&segment, sizeof(segment));
    if ((rw_result = read(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive conn req");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("incoming conn req: invalid checksum");

    // check that everything is set properly
    if (!mytcp_check_flag(segment, FLAG_SYN)) return abort_with_message("incoming conn req: SYN not set");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "incoming connection request");

    /* SEND CONNECTION GRANTED SEGMENT */

    // save client sequence
    uint32_t client_seq = segment.sequence;

    // re-initialize segment (assigns sequence number automatically)
    segment = mytcp_create_segment(SERVER_PORT, CLIENT_PORT);
    uint32_t server_seq = segment.sequence;

    // set acknowledgement number to client sequence number + 1
    segment.acknowledgment = client_seq + 1;

    // set SYNACK and checksum
    mytcp_set_flag(&segment, FLAG_SYN);
    mytcp_set_flag(&segment, FLAG_ACK);
    mytcp_set_checksum(&segment);

    // send connection granted response
    printf("sending connection granted ... ");
    if ((rw_result = write(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "connection granted");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing connection granted");

    /* RECEIVE CONNECTION ACKNOWLEDGMENT SEGMENT */

    printf("awaiting connection acknowledgment ... ");

    // attempt to receive connection ack
    bzero(&segment, sizeof(segment));
    if ((rw_result = read(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive ack");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("incoming ack: invalid checksum");

    // check values
    if (segment.sequence != client_seq + 1) return abort_with_message("incoming ack: sequence != initial_seq + 1");
    if (segment.acknowledgment != server_seq + 1) return abort_with_message("incoming ack: ack != server_seq + 1");
    if (!mytcp_check_flag(segment, FLAG_ACK)) return abort_with_message("incoming ack: ACK flag not set");

    printf("OK\n\nall good: we are now connected.\n");

    mytcp_print_segment(outfile, segment, "incoming connection acknowledgment");

    return 0;
}

int mock_close(int client_fd, FILE *outfile)
{
    printf("simulating closing a TCP connection\n\n");
    errno = 0;

    mytcp_t segment;
    bzero(&segment, sizeof(segment));

    /* RECEIVE CLOSE REQUEST */

    printf("awaiting close request ... ");

    ssize_t rw_result;
    if ((rw_result = read(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive close request");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("incoming close req: invalid checksum");

    // verify values
    uint32_t client_seq = segment.sequence;
    if (segment.acknowledgment != 0) return abort_with_message("incoming close req: non-zero ack number");
    if (!mytcp_check_flag(segment, FLAG_FIN)) return abort_with_message("incoming close req: FIN not set");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "incoming close request");

    /* SEND CLOSE ACK */

    printf("sending close ack ... ");

    // create a new segment (assigns sequence number automatically)
    segment = mytcp_create_segment(SERVER_PORT, CLIENT_PORT);
    uint32_t server_seq = segment.sequence;

    // set acknowledgment number to client_seq + 1
    segment.acknowledgment = client_seq + 1;
    mytcp_set_flag(&segment, FLAG_ACK);

    // send close ack
    mytcp_set_checksum(&segment);
    if ((rw_result = write(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "send close ack");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing close acknowledgment");

    /* SEND CLOSE REQUEST */

    printf("sending close request ... ");

    // clear ACK and set FIN
    mytcp_clear_flag(&segment, FLAG_ACK);
    mytcp_set_flag(&segment, FLAG_FIN);

    // send close request
    mytcp_set_checksum(&segment);
    if ((rw_result = write(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "send close request");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing close request");

    /* RECEIVE CLOSE ACK */

    printf("awaiting close ack ... ");

    // receive close ack
    bzero(&segment, sizeof(segment));
    if ((rw_result = read(client_fd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive close ack");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("receive close ack: invalid checksum");

    // verify values
    if (segment.sequence != client_seq + 1) return abort_with_message("close ack: sequence != client_seq + 1");
    if (segment.acknowledgment != server_seq + 1) return abort_with_message("close ack: ack != server_seq + 1");
    if (!mytcp_check_flag(segment, FLAG_ACK)) return abort_with_message("close ack: ACK not set");

    printf("OK\n\nall good. we have disconnected.\n");

    mytcp_print_segment(outfile, segment, "incoming close acknowledgment");

    return 0;
}
