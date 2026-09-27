/*
 * ipv4extract.c
 *
 * Reads lines of text from the user and extracts a single valid IPv4
 * address (optionally followed by a :port) embedded anywhere in the line.
 *
 * Build:   make
 * Run:     ./ipv4extract   (or "make run")
 * Clean:   make clean
 *
 * NOTE: extractIPv4() below is intentionally left as a stub with comments
 * describing what it needs to do. Fill in the logic yourself.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* Adjust if you expect longer input lines. fgets() will simply stop
 * reading at this many bytes (including the terminating newline). */
#define MAX_LINE_LEN 1024

/*
 * extractIPv4
 * -----------
 * Returns 1 if a valid address was found, 0 otherwise.
 * On success: *outAddress holds the 32-bit value, and
 *             *outPort holds the port number, or -1 if no port was present.
 * On failure: *outAddress is set to 0 and *outPort is set to -1.
 *
 * ---- Implementation notes / suggested approach ----
 *
 * 1. Scan through `str` looking for "candidate tokens": maximal runs of
 *    characters that are digits, '.', or ':'. Every other character is
 *    garbage that separates candidates.
 *    Example: "192a168.1.1.1" contains two candidate tokens: "192" and
 *    "168.1.1.1" (the 'a' breaks the run).
 *
 * 2. For each candidate token, in left-to-right order, try to match it
 *    AGAINST THE WHOLE GRAMMAR BELOW as a single unit. A token must match
 *    completely, start to end -- no truncating a longer/invalid run down
 *    to a valid-looking piece inside it.
 *    Example: "192.168.1.1." is one token (trailing '.' included) and
 *    must be rejected entirely, even though "192.168.1.1" alone would
 *    have been valid.
 *
 *      address := octet '.' octet '.' octet '.' octet [ ':' port ]
 *      octet   := 1-3 digits, numeric value 0-255,
 *                 no leading '0' unless the octet is exactly "0"
 *      port    := 1-5 digits, numeric value 0-65535,
 *                 no leading '0' unless the port is exactly "0"
 *
 *    If a ':' is present, the port must be fully valid, or the ENTIRE
 *    token (address included) is rejected -- do not fall back to
 *    reporting the address without a port in that case.
 *
 * 3. Do all digit accumulation by hand (no atoi/strtol/sscanf/etc. --
 *    see the assignment's list of disallowed functions). Watch for
 *    overflow: e.g. bail out of an octet/port as soon as its
 *    accumulated value cannot possibly stay in range, rather than
 *    letting an int overflow.
 *
 * 4. On the first candidate token that fully validates: compute the
 *    32-bit address value as (A<<24 | B<<16 | C<<8 | D), set *outPort
 *    to the parsed port (or -1 if none), and return 1.
 *
 * 5. If no candidate token in the whole string validates, set
 *    *outAddress = 0, *outPort = -1, and return 0.
 *
 * You may find it useful to write a small helper that attempts to parse
 * one candidate token (given a pointer and length) and reports
 * success/failure plus the parsed values, so extractIPv4() can just loop
 * over candidate tokens and call the helper on each.
 */
int extractIPv4(const char* str, unsigned long* outAddress, int* outPort) {
    /* Defaults for the failure case; overwrite these on success. */
    *outAddress = 0;
    *outPort = -1;

    /* TODO: replace this with your implementation. */
    (void)str;

    return 0;
}

int main(void) {
    char line[MAX_LINE_LEN];

    while (1) {
        printf("Enter a string (or 'END' to quit): ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            /* EOF or read error on stdin. */
            break;
        }

        /* Strip a trailing newline (and, just in case, a preceding '\r'
         * for CRLF input) left in the buffer by fgets(). */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (strcmp(line, "END") == 0) {
            printf("Program terminated.\n");
            break;
        }

        unsigned long address = 0;
        int port = -1;
        int found = extractIPv4(line, &address, &port);

        if (found) {
            unsigned int a = (unsigned int)((address >> 24) & 0xFF);
            unsigned int b = (unsigned int)((address >> 16) & 0xFF);
            unsigned int c = (unsigned int)((address >> 8) & 0xFF);
            unsigned int d = (unsigned int)(address & 0xFF);

            if (port == -1) {
                printf("Extracted IPv4 address: %u.%u.%u.%u (decimal value: %lu, port: none)\n",
                       a, b, c, d, address);
            } else {
                printf("Extracted IPv4 address: %u.%u.%u.%u (decimal value: %lu, port: %d)\n",
                       a, b, c, d, address, port);
            }
        } else {
            printf("Invalid input: no valid IPv4 address found\n");
        }
    }

    return 0;
}
