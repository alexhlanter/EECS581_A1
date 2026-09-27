/*
 * ipv4extract.c
 *
 * Reads a line of text from the user and extracts a single valid IPv4
 * address (optionally followed by a valid :port) embedded anywhere in the line.
 *
 * Build:   make
 * Run:     ./ipv4extract   (or "make run")
 * Clean:   make clean

 Claude wrote multiline comments, I wrote // single lines. All code is from Claude.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Starting size for the dynamic line buffer; it doubles as needed, so
 * this is just a reasonable first guess, not a hard limit. Lines (e.g.
 * long firewall log entries) can be arbitrarily long.
 * Had Claude make it this way since the "real world relevance" talked about this and could mean long tests are used.
 */
#define INITIAL_LINE_CAPACITY 256

/*
 * readLine
 * --------
 * Reads one line from `in`, growing a heap buffer as needed so there is
 * no fixed cap on line length. The trailing '\n' (and a preceding '\r',
 * for CRLF input) is stripped and not included in the result.
 *
 * Returns a malloc'd, NUL-terminated string (caller must free() it), or
 * NULL on end-of-file with nothing read, or on allocation failure.
 */
static char* readLine(FILE* in) { 
    size_t capacity = INITIAL_LINE_CAPACITY;
    size_t length = 0;
    char* buffer = (char*)malloc(capacity);
    if (!buffer) { // if malloc fails return NULL
        return NULL;
    }

    while (1) { // loop, if length about to hit capacity double the buffer
        if (length + 1 >= capacity) {
            size_t newCapacity = capacity * 2;
            char* newBuffer = (char*)realloc(buffer, newCapacity);
            if (!newBuffer) {
                free(buffer);
                return NULL; //if malloc fails return NULL
            }
            buffer = newBuffer;
            capacity = newCapacity;
        }

        int ch = fgetc(in); //read character
        if (ch == EOF) {
            if (length == 0) { //If file over return NULL if never read anything
                free(buffer);
                return NULL;
            }
            break;
        }
        if (ch == '\n') { //if newline break loop (skip character)
            break;
        }
        buffer[length++] = (char)ch; //add character to buffer, increase length
    }

    if (length > 0 && buffer[length - 1] == '\r') { //if there is a \r at the end get rid of it
        length--;
    }

    buffer[length] = '\0'; //null terminator at end of buffer then return it.
    return buffer;
}

/*
 * parseOctet
 * ----------
 * Parses one address octet starting at tok[*pos]: 1-3 digits, value
 * 0-255, no leading zero unless the octet is exactly "0".
 * On success, advances *pos past the digits, stores the value in
 * *value, and returns 1. On failure, *pos is left in an unspecified
 * position and 0 is returned (the caller only continues on success).
 */
static int parseOctet(const char* tok, int len, int* pos, int* value) {
    int start = *pos;
    int digits = 0;
    int val = 0;

    while (*pos < len && tok[*pos] >= '0' && tok[*pos] <= '9') { //read single digit, if there is 3 already fail
        if (digits == 3) {
            return 0;
        }
        val = val * 10 + (tok[*pos] - '0'); //put digit in right place by mult by 10 based on position
        digits++;
        (*pos)++;
        if (val > 255) { //If out of range fail
            return 0;
        }
    }

    if (digits == 0) {
        return 0; /* empty octet */
    }
    if (digits > 1 && tok[start] == '0') {
        return 0; /* disallowed leading zero */
    }

    *value = val;
    return 1;
}

/*
 * parsePort
 * ---------
 * Parses a port number starting at tok[*pos]: 1-5 digits, value
 * 0-65535, no leading zero unless the port is exactly "0".
 * Same success/failure contract as parseOctet.
 */
static int parsePort(const char* tok, int len, int* pos, int* value) {
    int start = *pos;
    int digits = 0;
    long val = 0;

    while (*pos < len && tok[*pos] >= '0' && tok[*pos] <= '9') { //read digit, if more than 5 fail
        if (digits == 5) {
            return 0;
        }
        val = val * 10 + (tok[*pos] - '0'); //track value by mult 10 with position
        digits++;
        (*pos)++;
        if (val > 65535) { //If out of range fail
            return 0;
        }
    }

    if (digits == 0) { //If nothing fail
        return 0;
    }
    if (digits > 1 && tok[start] == '0') { //if leading 0 fail
        return 0;
    }

    *value = (int)val;
    return 1;
}

/*
 * tryParseToken
 * -------------
 * Attempts to match the ENTIRE token tok[0..len) against:
 *   octet '.' octet '.' octet '.' octet [ ':' port ]
 * with nothing left over. Returns 1 and fills outAddress/outPort on
 * a full match, 0 otherwise (leaving the outputs untouched).
 */
static int tryParseToken(const char* tok, int len, unsigned long* outAddress, int* outPort) {
    int pos = 0;
    int o1, o2, o3, o4;

    if (!parseOctet(tok, len, &pos, &o1)) return 0; //for octets if cant parse octet, run out of characters, or not separated by . fail
    if (pos >= len || tok[pos] != '.') return 0;
    pos++;

    if (!parseOctet(tok, len, &pos, &o2)) return 0;
    if (pos >= len || tok[pos] != '.') return 0;
    pos++;

    if (!parseOctet(tok, len, &pos, &o3)) return 0;
    if (pos >= len || tok[pos] != '.') return 0;
    pos++;

    if (!parseOctet(tok, len, &pos, &o4)) return 0;

    int port = -1;
    if (pos < len) { // if more characters to go has to have a port and then one that passes parse
        /* Anything left over after the 4th octet must be exactly
         * ':' followed by a fully valid port, and nothing more. */
        if (tok[pos] != ':') return 0;
        pos++;
        if (!parsePort(tok, len, &pos, &port)) return 0;
    }

    if (pos != len) { //extract sends this function digits, dots and colon. If there is extra chars, fail.
        return 0;
    }

    *outAddress = ((unsigned long)o1 << 24) | ((unsigned long)o2 << 16) | // convert octets into 32 bit number
                  ((unsigned long)o3 << 8) | (unsigned long)o4;
    *outPort = port; 
    return 1; //success
}

/*
 * extractIPv4
 * -----------
 * Returns 1 if a valid address was found, 0 otherwise.
 * On success: *outAddress holds the 32-bit value, and
 *             *outPort holds the port number, or -1 if no port was present.
 * On failure: *outAddress is set to 0 and *outPort is set to -1.
 *
 * Scans the string for maximal runs of characters in {digit, '.', ':'}
 * ("candidate tokens"). Every other character is garbage that separates
 * candidates. Each candidate is tried, in left-to-right order, as a
 * complete match against the address[:port] grammar; the first one that
 * matches in full wins. A candidate that fails is not truncated or
 * re-tried piecemeal -- it's simply skipped in its entirety.
 */
int extractIPv4(const char* str, unsigned long* outAddress, int* outPort) {
    *outAddress = 0;
    *outPort = -1;

    int i = 0;
    while (str[i] != '\0') { //While haven't hit null terminator, read digits dots and colon. Increase position as you go.
        char c = str[i];
        if ((c >= '0' && c <= '9') || c == '.' || c == ':') {
            int start = i;
            while (str[i] != '\0' &&
                   ((str[i] >= '0' && str[i] <= '9') || str[i] == '.' || str[i] == ':')) {
                i++;
            }
            int len = i - start;

            unsigned long addr;
            int port;
            if (tryParseToken(str + start, len, &addr, &port)) { //try to parse with read token
                *outAddress = addr;
                *outPort = port;
                return 1;
            }
            /* Candidate failed; continue scanning right after it. */
        } else {
            i++; //move up i if garbage character
        }
    }

    return 0;
}

int main(void) {
    while (1) { //running loop
        printf("Enter a string (or 'END' to quit): ");
        fflush(stdout);

        char* line = readLine(stdin);
        if (!line) {
            /* EOF or allocation failure on stdin. */
            break;
        }

        if (strcmp(line, "END") == 0) { //break running loop if exactly END is entered. Free memory.
            printf("Program terminated.\n");
            free(line);
            break;
        }

        unsigned long address = 0;
        int port = -1;
        int found = extractIPv4(line, &address, &port); //Extract ipv4+port

        if (found) { //if get ip, separate out each octet with logic operations
            unsigned int a = (unsigned int)((address >> 24) & 0xFF);
            unsigned int b = (unsigned int)((address >> 16) & 0xFF);
            unsigned int c = (unsigned int)((address >> 8) & 0xFF);
            unsigned int d = (unsigned int)(address & 0xFF);

            if (port == -1) { //no port
                printf("Extracted IPv4 address: %u.%u.%u.%u (decimal value: %lu, port: none)\n",
                       a, b, c, d, address);
            } else { //yes port
                printf("Extracted IPv4 address: %u.%u.%u.%u (decimal value: %lu, port: %d)\n",
                       a, b, c, d, address, port);
            }
        } else { //extract failed
            printf("Invalid input: no valid IPv4 address found\n");
        }

        free(line); //free memory
    }

    return 0; //running loop is broken, end program
}
