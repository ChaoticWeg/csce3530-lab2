#ifndef CSCE3530_LAB3_MYTCP_H
#define CSCE3530_LAB3_MYTCP_H

#define FLAG_FIN   0
#define FLAG_SYN   1
#define FLAG_RST   2
#define FLAG_PSH   3
#define FLAG_ACK   4
#define FLAG_URG   5
#define NUM_FLAGS  6

#define MASK_OFFSET 0xF000
#define MASK_RESERVED 0x0FC0

#define MAX_TCP_CHAR_SIZE 2048

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

// TCP header structure
struct mytcp
{
    uint16_t srcport;
    uint16_t destport;
    uint32_t sequence;
    uint32_t acknowledgment;
    uint16_t flags;     // <---  also contains offset and reserved
    uint16_t receive;
    uint16_t checksum;
    uint16_t urgent;
    uint32_t options;
} __attribute__((packed));

// typedef above struct as mytcp_t
typedef struct mytcp mytcp_t;

// flag names as arrays to make printing easier later
char *FLAG_NAMES[6];

// generate random sequence number
uint32_t mytcp_generate_sequence();

// tcp segment creation
mytcp_t mytcp_create_segment(uint16_t, uint16_t);

// set/clear header flags
void mytcp_set_flag(mytcp_t *, uint8_t);
void mytcp_clear_flag(mytcp_t *, uint8_t);
bool mytcp_check_flag(const mytcp_t, uint8_t);

// calculate checksum
uint16_t mytcp_calculate_checksum(mytcp_t);
void mytcp_set_checksum(mytcp_t *);
bool mytcp_verify_checksum(const mytcp_t);

// print segment
void mytcp_print_segment(FILE *, const mytcp_t, const char *);

#endif //CSCE3530_LAB3_MYTCP_H
