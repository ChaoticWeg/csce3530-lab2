#include "mytcp.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>


// flag names as array to make printing easier later
char *FLAG_NAMES[6] = { "FIN", "SYN", "RST", "PSH", "ACK", "URG" };


/**
 * Generate a random initial sequence number, leaving room for adding 1.
 *
 * @return A pseudo-randomly generated 32-bit integer between 0 and 4.2E9
 */
uint32_t mytcp_generate_sequence()
{
    return ((uint32_t) rand()) % 0xFFFFFFF0;
}

/**
 * Create a TCP header segment, given a source and destination port.
 * Automatically fills the sequence number.
 *
 * @param srcport Source port
 * @param destport Destination port
 * @return A segment with srcport, destport, sequence, and header length set properly and everything else set to zero
 */
mytcp_t mytcp_create_segment(uint16_t srcport, uint16_t destport)
{
    mytcp_t result;

    // initialize all fields to zero
    bzero(&result, sizeof(result));

    // set port and sequence numbers accordingly
    result.srcport = srcport;
    result.destport = destport;
    result.sequence = mytcp_generate_sequence();

    // set offset properly in the flags; it will not change
    result.flags = (sizeof(mytcp_t) / 4) << 12;

    return result;
}

/**
 * Set a segment's flag. Can only set a single flag per call to mytcp_set_flag().
 * Flags are defined as macros in mytcp.h.
 *
 * @param segment The segment whose flags we are modifying
 * @param offset The (single!) flag to set, e.g. FLAG_ACK
 */
void mytcp_set_flag(mytcp_t *segment, uint8_t offset)
{
    segment->flags |= 1 << offset;
}

/**
 * Clear a segment's flag. Can only clear a single flag per call to mytcp_clear_flag().
 *
 * @param segment The segment whose flags we are modifying
 * @param offset The (single!) flag to clear, e.g. FLAG_SYN
 */
void mytcp_clear_flag(mytcp_t *segment, uint8_t offset)
{
    segment->flags &= 0 << offset;
}

/**
 * Check whether a particular flag is set for a segment. Can only check a single flag per call to mytcp_check_flag().
 *
 * @param segment The segment whose flags we are checking
 * @param offset The (single!) flag to check, e.g. FLAG_FIN
 * @return True iff the flag is set, else false
 */
bool mytcp_check_flag(const mytcp_t segment, uint8_t offset)
{
    return (segment.flags & (1 << offset)) > 0;
}

/**
 * Calculate the checksum of a mytcp_t segment as if its prior checksum value was zero.
 * Creates a copy with a zero checksum, whose checksum we calculate.
 *
 * Checksum calculation algorithm was provided with the assignment requirements.
 *
 * @param segment The segment whose checksum we are calculating
 * @return The checksum of the segment, truncated to fit into a 16-bit integer
 */
uint16_t mytcp_calculate_checksum(mytcp_t segment)
{
    // initialize calculation values
    uint16_t checksum_arr[12];
    uint32_t i, checksum, sum = 0;

    // copy 24 bytes to the checksum array
    memcpy(checksum_arr, &segment, 24);

    // compute the sum
    for (i = 0; i < 12; i++) sum += checksum_arr[i];

    // fold once
    checksum = sum >> 16;
    sum &= 0xFFFF;
    sum += checksum;

    // fold again
    checksum = sum >> 16;
    sum &= 0xFFFF;
    checksum += sum;

    return (uint16_t) (0xFFFF ^ checksum);
}

/**
 * Calculate and populate the checksum value for a segment.
 *
 * @param seg The segment whose checksum we are calculating and populating
 */
void mytcp_set_checksum(mytcp_t *seg)
{
    seg->checksum = 0;
    seg->checksum = mytcp_calculate_checksum(*seg);
}

/**
 * Calculate a segment's checksum and verify against the prior checksum given.
 * Does not modify the segment in any way; creates a copy with a zero-checksum.
 *
 * @param seg The segment whose checksum we are verifying
 * @return True iff the segment's checksum is correct, else false
 */
bool mytcp_verify_checksum(const mytcp_t seg)
{
    uint16_t given = seg.checksum;

    // create a copy and set its checksum to zero
    mytcp_t test;
    bzero(&test, sizeof(mytcp_t));
    memcpy(&test, &seg, sizeof(seg));
    test.checksum = 0;

    // calculate expected checksum
    uint16_t expected = mytcp_calculate_checksum(test);
    return (bool) (expected == given);
}

/**
 * Print a segment to file, as well as stdout.
 * Complies with assignment requirements.
 *
 * @param f The file to print the segment to
 * @param seg The segment to print to file and stdout
 * @param title The title to give this segment, e.g. "incoming connection request"
 */
void mytcp_print_segment(FILE *f, const mytcp_t seg, const char *title)
{
    // title and separator
    char sep[strlen(title) + 1];
    bzero(sep, strlen(title) + 1);
    for (int i = 0; title[i] != '\0'; i++) sep[i] = '-';

    // binary representation of flags
    char flags[NUM_FLAGS + 1];
    bzero(flags, NUM_FLAGS + 1);
    for (uint8_t i = FLAG_FIN; i < NUM_FLAGS; i++)
        flags[FLAG_URG - i] = (char) (mytcp_check_flag(seg, i) ? '1' : '0');

    // names of flags
    size_t max_flag_names_len = (FLAG_URG * 4) + 1;
    char flag_names[max_flag_names_len];
    bzero(flag_names, max_flag_names_len);
    for (uint8_t i = 0; i < NUM_FLAGS; i++)
    {
        if (mytcp_check_flag(seg, i))
        {
            // copy this flag name into flag names
            char *flag_name = FLAG_NAMES[i];
            snprintf(flag_names, max_flag_names_len, "%s %s", flag_names, flag_name);
        }
    }

    // extract reserved from 16-bit that also holds flags and offset
    uint8_t reserved = (uint8_t) ((seg.flags & MASK_RESERVED) >> 6);

    // extract offset from 16-bit that also holds flags and reserved
    char offset = (char) ((seg.flags & MASK_OFFSET) >> 12);

    // hold in a string so we don't have to format twice
    char *str = (char *) malloc(MAX_TCP_CHAR_SIZE);
    bzero(str, MAX_TCP_CHAR_SIZE);
    snprintf(str, MAX_TCP_CHAR_SIZE,
             "%s\n%s\n"
             "srcport:         %d\n"
             "destport:        %d\n"
             "sequence:        %d\n"
             "acknowledgment:  %d\n"
             "offset:          %d\n"
             "reserved:        0x%06X\n"
             "flags:           0b%s%s\n"
             "receive:         0x%04X\n"
             "checksum:        0x%04X\n"
             "urgent:          0x%04X\n"
             "options:         0x%08X\n",
             title, sep,
             seg.srcport, seg.destport, seg.sequence, seg.acknowledgment,
             offset, reserved, flags, flag_names,
             seg.receive, seg.checksum, seg.urgent, seg.options
    );

    // print to file, then print to stdout
    fprintf(f, "%s\n", str);
    fprintf(stdout, "%s\n", str);
}
