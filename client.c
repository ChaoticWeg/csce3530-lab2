#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define OUTFILE_NAME "client.out"

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
    if (sockfd == -1 || errno != 0) return abort_with_errno(errno, "socket");

    // resolve hostname
    struct hostent *server_hostname = gethostbyname(SERVER_HOSTNAME);
    if (server_hostname == NULL) return abort_with_message("Error: invalid hostname");

    // build server address
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *((struct in_addr *) server_hostname->h_addr_list[0]);
    server_addr.sin_port = htons(SERVER_PORT);

    // attempt to connect
    printf("connecting to %s:%d ... ", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0 || errno != 0)
        return abort_with_errno(errno, "connect");

    // we have connected
    printf("OK\n");

    // prepare to branch
    int result = 1;

    // if we are opening, mock open
    if (strcasecmp(argv[1], "open") == 0)
        result = mock_open(sockfd, outfile);

        // if we are closing, mock close
    else if (strcasecmp(argv[1], "close") == 0)
        result = mock_close(sockfd, outfile);

    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return result;
}

int mock_open(int sockfd, FILE *outfile)
{
    printf("simulating opening a TCP connection\n\n");

    errno = 0;

    /* SEND CONNECTION REQUEST SEGMENT */

    printf("sending connection request ... ");

    // create a tcp segment with initial values
    mytcp_t segment = mytcp_create_segment(CLIENT_PORT, SERVER_PORT);
    uint32_t initial_seq = segment.sequence;
    mytcp_set_flag(&segment, FLAG_SYN);
    mytcp_set_checksum(&segment);

    // attempt to send
    ssize_t rw_result;
    if ((rw_result = write(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "send conn req");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing connection request");

    /* RECEIVE CONNECTION GRANTED SEGMENT */

    printf("awaiting connection granted ... ");

    // receive connection granted segment
    if ((rw_result = read(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive conn granted");

    // check connection granted segment
    if (segment.acknowledgment != initial_seq + 1) return abort_with_message("conn granted: ack != initial_seq + 1");
    if (!mytcp_check_flag(segment, FLAG_SYN)) return abort_with_message("conn granted: SYN not set");
    if (!mytcp_check_flag(segment, FLAG_ACK)) return abort_with_message("conn granted: ACK not set");
    if (!mytcp_verify_checksum(segment)) return abort_with_message("conn granted: invalid checksum");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "incoming connection granted");

    /* SEND CONNECTION ACKNOWLEDGEMENT SEGMENT */

    // set sequence to initial sequence + 1, acknowledgement to server sequence + 1
    uint32_t server_seq = segment.sequence;
    segment.sequence = initial_seq + 1;
    segment.acknowledgment = server_seq + 1;

    // set ACK flag and checksum
    mytcp_set_flag(&segment, FLAG_ACK);
    mytcp_set_checksum(&segment);

    // send connection acknowledgement segment
    printf("sending connection acknowledgement ... ");
    if ((rw_result = write(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "send conn ack");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing connection acknowledgment");

    return 0;
}

int mock_close(int sockfd, FILE *outfile)
{
    printf("simulating opening a TCP connection\n\n");

    errno = 0;

    /* SEND CLOSE REQUEST */

    printf("sending close request ... ");

    // create a new TCP segment with initial values
    mytcp_t segment = mytcp_create_segment(CLIENT_PORT, SERVER_PORT);
    uint32_t initial_seq = segment.sequence;
    mytcp_set_flag(&segment, FLAG_FIN);
    mytcp_set_checksum(&segment);

    // attempt to send
    ssize_t rw_result;
    if ((rw_result = write(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "request close");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing close request");

    /* RECEIVE ACKNOWLEDGMENT SEGMENT */

    printf("awaiting close acknowledgment ... ");

    if ((rw_result = read(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive close ack 1");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("ack 1: invalid checksum");

    // verify values
    uint32_t server_seq = segment.sequence;
    if (segment.acknowledgment != initial_seq + 1) return abort_with_message("ack 1: bad ack number");
    if (!mytcp_check_flag(segment, FLAG_ACK)) return abort_with_message("ack 1: ACK not set");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "incoming close acknowledgment");

    /* RECEIVE ANOTHER CLOSE ACKNOWLEDGMENT SEGMENT */

    printf("awaiting close request ... ");

    if ((rw_result = read(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "receive close ack 2");

    // verify checksum
    if (!mytcp_verify_checksum(segment)) return abort_with_message("ack 2: invalid checksum");

    // verify values
    if (segment.acknowledgment != initial_seq + 1) return abort_with_message("ack 2: bad ack number");
    if (!mytcp_check_flag(segment, FLAG_FIN)) return abort_with_message("ack 2: FIN not set");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "incoming close request");

    /* SEND CLOSE ACKNOWLEDGMENT SEGMENT */

    printf("sending close acknowledgment ... ");

    // set values
    segment = mytcp_create_segment(CLIENT_PORT, SERVER_PORT);
    segment.sequence = initial_seq + 1;
    segment.acknowledgment = server_seq + 1;
    mytcp_set_flag(&segment, FLAG_ACK);
    mytcp_set_checksum(&segment);

    // send ack
    if ((rw_result = write(sockfd, &segment, sizeof(segment))) != sizeof(segment) || errno != 0)
        return handle_bad_rw_result(rw_result, "send final close ack");

    printf("OK\n");

    mytcp_print_segment(outfile, segment, "outgoing close acknowledgment");

    return 0;
}
