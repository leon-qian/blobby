// blobby.c
// blob file archiver
// COMP1521 20T3 Assignment 2
// Written by <LEON QIAN>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// the first byte of every blobette has this value
#define BLOBETTE_MAGIC_NUMBER 0x42

// number of bytes in fixed-length blobette fields
#define BLOBETTE_MAGIC_NUMBER_BYTES 1
#define BLOBETTE_MODE_LENGTH_BYTES 3
#define BLOBETTE_PATHNAME_LENGTH_BYTES 2
#define BLOBETTE_CONTENT_LENGTH_BYTES 6
#define BLOBETTE_HASH_BYTES 1

// maximum number of bytes in variable-length blobette fields
#define BLOBETTE_MAX_PATHNAME_LENGTH 65535
#define BLOBETTE_MAX_CONTENT_LENGTH 281474976710655

// ADD YOUR #defines HERE
#define BLOBETTE_HASH_INITIAL_VALUE 0

typedef enum action { a_invalid, a_list, a_extract, a_create } action_t;

// typedef struct blobette {
//     mode_t file_mode;
//     uint16_t pathname_length;
//     uint64_t content_length;
//     uint8_t hash_byte;
// } blobette_t;

void usage(char *myname);
action_t process_arguments(int argc, char *argv[], char **blob_pathname,
                           char ***pathnames, int *compress_blob);

void list_blob(char *blob_pathname);
void extract_blob(char *blob_pathname);
void create_blob(char *blob_pathname, char *pathnames[], int compress_blob);

uint8_t blobby_hash(uint8_t hash, uint8_t byte);

// ADD YOUR FUNCTION PROTOTYPES HERE

/**
 * Create a new blobette.
 */
// blobette_t new_blobette(mode_t file_mode, uint16_t pathname_length,
//                         uint64_t content_length, uint8_t hash_byte) {
//     return (blobette_t){.file_mode = file_mode,
//                         .pathname_length = pathname_length,
//                         .content_length = content_length,
//                         .hash_byte = hash_byte};
// }

/**
 * Encode a blobette and write binary data to file stream.
 *
 * Return 0 on success,
 *        1 on failure.
 */
// int encode_blobette(FILE *stream, blobette_t blobette) {}

/**
 * Read binary data from file stream and decode a blobette.
 *
 * Return 0 on success,
 *        1 on failure.
 */
// int decode_blobette(blobette_t *blobette, FILE *stream) {}

/**
 * Read the next byte in the stream and update the hash.
 *
 * Return 0 on success,
 *        1 on failure.
 *
 * Note: Reaching EOF is considered a failure.
 */
int process_next_byte(FILE *stream, uint8_t *byte, uint8_t *hash) {
    int temp = fgetc(stream);
    if (temp == EOF) {
        return 1;
    }

    *byte = temp;
    // if (hash) printf("hash: %02x\n", *hash);
    // printf("byte: %02x\n", *byte);
    if (hash) *hash = blobby_hash(*hash, *byte);
    // if (hash) printf("\tblobby_hash: %02x\n\n", *hash);
    return 0;
}

/**
 * Pack an array of blobettes into a blob file.
 *
 * Return 0 on success,
 *        1 on failure.
 *
 * On failure, the blob file will contain all the packed blobettes up to the
 * point of failure. Note that this file will be an invalid blob file.
 */
// int pack_blob(blobette_t *blobettes, int n_blobettes, char *blob_pathname) {}

/**
 * Unpack a blob file into an array of blobettes.
 *
 * Return 0 on success,
 *        1 on failure.
 *
 * On failure, the array of blobettes will contain all the unpacked blobettes up
 * to the point of failure. Note that the last blobette in the array will be an
 * invalid blobette.
 */
void unpack_blob(char *blob_pathname, int depth) {
    // Open the blob.
    FILE *blob_stream = fopen(blob_pathname, "r");
    if (!blob_stream) {
        perror(blob_pathname);
        exit(EXIT_FAILURE);
    }

    uint8_t hash = BLOBETTE_HASH_INITIAL_VALUE;
    uint8_t byte;
    while (!process_next_byte(blob_stream, &byte, &hash)) {
        // check magic number
        if (byte != BLOBETTE_MAGIC_NUMBER) {
            fprintf(stderr, "ERROR: Magic byte of blobette incorrect\n");
            exit(EXIT_FAILURE);
        }

        // get mode
        mode_t mode = 0;
        for (int i = 0; i < BLOBETTE_MODE_LENGTH_BYTES; ++i) {
            if (process_next_byte(blob_stream, &byte, &hash)) {
                fprintf(stderr, "ERROR: Incomplete mode\n");
                exit(EXIT_FAILURE);
            }

            mode <<= 8;
            mode |= byte;
        }

        // get pathname length
        uint16_t pathname_length = 0;
        for (int i = 0; i < BLOBETTE_PATHNAME_LENGTH_BYTES; ++i) {
            if (process_next_byte(blob_stream, &byte, &hash)) {
                fprintf(stderr, "ERROR: Incomplete pathname length\n");
                exit(EXIT_FAILURE);
            }

            pathname_length <<= 8;
            pathname_length |= byte;
        }

        // get content length
        uint64_t content_length = 0;
        for (int i = 0; i < BLOBETTE_CONTENT_LENGTH_BYTES; ++i) {
            if (process_next_byte(blob_stream, &byte, &hash)) {
                fprintf(stderr, "ERROR: Incomplete content length\n");
                exit(EXIT_FAILURE);
            }

            content_length <<= 8;
            content_length |= byte;
        }

        // get pathname bytes
        uint8_t *pathname = malloc(sizeof(uint8_t) * (pathname_length + 1));
        for (int i = 0; i < pathname_length; ++i) {
            if (process_next_byte(blob_stream, &byte, &hash)) {
                free(pathname);
                fprintf(stderr, "ERROR: Incomplete pathname\n");
                exit(EXIT_FAILURE);
            }

            pathname[i] = byte;
        }
        pathname[pathname_length] = '\0';

        if (!depth) {
            printf("%06o %5lu %s\n", mode, content_length, pathname);
            free(pathname);
            if (fseek(blob_stream, content_length + BLOBETTE_HASH_BYTES,
                      SEEK_CUR)) {
                perror(blob_pathname);
                exit(EXIT_FAILURE);
            }
        } else {
            // create files and write contents

            // get hash
            if (process_next_byte(blob_stream, &byte, NULL)) {
                fprintf(stderr, "ERROR: Hash of blobette not found\n");
                exit(EXIT_FAILURE);
            }

            // check hash
            if (byte != hash) {
                fprintf(stderr, "ERROR: Hash of blobette incorrect\n");
                exit(EXIT_FAILURE);
            }
        }

        // reset hash
        hash = BLOBETTE_HASH_INITIAL_VALUE;
    }

    fclose(blob_stream);
}

// YOU SHOULD NOT NEED TO CHANGE main, usage or process_arguments

int main(int argc, char *argv[]) {
    char *blob_pathname = NULL;
    char **pathnames = NULL;
    int compress_blob = 0;
    action_t action = process_arguments(argc, argv, &blob_pathname, &pathnames,
                                        &compress_blob);

    switch (action) {
        case a_list:
            list_blob(blob_pathname);
            break;

        case a_extract:
            extract_blob(blob_pathname);
            break;

        case a_create:
            create_blob(blob_pathname, pathnames, compress_blob);
            break;

        default:
            usage(argv[0]);
    }

    return 0;
}

// print a usage message and exit

void usage(char *myname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\t%s -l <blob-file>\n", myname);
    fprintf(stderr, "\t%s -x <blob-file>\n", myname);
    fprintf(stderr, "\t%s [-z] -c <blob-file> pathnames [...]\n", myname);
    exit(1);
}

// process command-line arguments
// check we have a valid set of arguments
// and return appropriate action
// **blob_pathname set to pathname for blobfile
// ***pathname set to a list of pathnames for the create action
// *compress_blob set to an integer for create action

action_t process_arguments(int argc, char *argv[], char **blob_pathname,
                           char ***pathnames, int *compress_blob) {
    extern char *optarg;
    extern int optind, optopt;
    int create_blob_flag = 0;
    int extract_blob_flag = 0;
    int list_blob_flag = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":l:c:x:z")) != -1) {
        switch (opt) {
            case 'c':
                create_blob_flag++;
                *blob_pathname = optarg;
                break;

            case 'x':
                extract_blob_flag++;
                *blob_pathname = optarg;
                break;

            case 'l':
                list_blob_flag++;
                *blob_pathname = optarg;
                break;

            case 'z':
                (*compress_blob)++;
                break;

            default:
                return a_invalid;
        }
    }

    if (create_blob_flag + extract_blob_flag + list_blob_flag != 1) {
        return a_invalid;
    }

    if (list_blob_flag && argv[optind] == NULL) {
        return a_list;
    } else if (extract_blob_flag && argv[optind] == NULL) {
        return a_extract;
    } else if (create_blob_flag && argv[optind] != NULL) {
        *pathnames = &argv[optind];
        return a_create;
    }

    return a_invalid;
}

/**
 * list the contents of blob_pathname
 * This function is called when the -l flag is set.
 *
 * file -> array of blobettes
 * print each blobette
 */
void list_blob(char *blob_pathname) {
    // REPLACE WITH YOUR CODE FOR -l

    // printf("list_blob called to list '%s'\n", blob_pathname);

    // HINT: you'll need a printf like:
    // printf("%06lo %5lu %s\n", mode, content_length, pathname);

    unpack_blob(blob_pathname, 0);
}

/**
 * extract the contents of blob_pathname
 * This function is called when the -x flag is set.
 *
 * file -> array of blobettes
 * print each blobette name
 * blobette -> file
 */
void extract_blob(char *blob_pathname) {
    // REPLACE WITH YOUR CODE FOR -x

    printf("extract_blob called to extract '%s'\n", blob_pathname);
}

// create blob_pathname from NULL-terminated array pathnames
// compress with xz if compress_blob non-zero (subset 4)
/**
 * This function is called when the -c flag is set.
 * This function compresses blob if -z flag is set.
 *
 * files -> array of blobettes
 * print each blobette name
 * blobettes -> blob
 */
void create_blob(char *blob_pathname, char *pathnames[], int compress_blob) {
    // REPLACE WITH YOUR CODE FOR -c

    printf("create_blob called to create %s blob '%s' containing:\n",
           compress_blob ? "compressed" : "non-compressed", blob_pathname);

    for (int p = 0; pathnames[p]; p++) {
        printf("%s\n", pathnames[p]);
    }
}

// ADD YOUR FUNCTIONS HERE

// YOU SHOULD NOT CHANGE CODE BELOW HERE

// Lookup table for a simple Pearson hash
// https://en.wikipedia.org/wiki/Pearson_hashing
// This table contains an arbitrary permutation of integers 0..255

const uint8_t blobby_hash_table[256] = {
    241, 18,  181, 164, 92,  237, 100, 216, 183, 107, 2,   12,  43,  246, 90,
    143, 251, 49,  228, 134, 215, 20,  193, 172, 140, 227, 148, 118, 57,  72,
    119, 174, 78,  14,  97,  3,   208, 252, 11,  195, 31,  28,  121, 206, 149,
    23,  83,  154, 223, 109, 89,  10,  178, 243, 42,  194, 221, 131, 212, 94,
    205, 240, 161, 7,   62,  214, 222, 219, 1,   84,  95,  58,  103, 60,  33,
    111, 188, 218, 186, 166, 146, 189, 201, 155, 68,  145, 44,  163, 69,  196,
    115, 231, 61,  157, 165, 213, 139, 112, 173, 191, 142, 88,  106, 250, 8,
    127, 26,  126, 0,   96,  52,  182, 113, 38,  242, 48,  204, 160, 15,  54,
    158, 192, 81,  125, 245, 239, 101, 17,  136, 110, 24,  53,  132, 117, 102,
    153, 226, 4,   203, 199, 16,  249, 211, 167, 55,  255, 254, 116, 122, 13,
    236, 93,  144, 86,  59,  76,  150, 162, 207, 77,  176, 32,  124, 171, 29,
    45,  30,  67,  184, 51,  22,  105, 170, 253, 180, 187, 130, 156, 98,  159,
    220, 40,  133, 135, 114, 147, 75,  73,  210, 21,  129, 39,  138, 91,  41,
    235, 47,  185, 9,   82,  64,  87,  244, 50,  74,  233, 175, 247, 120, 6,
    169, 85,  66,  104, 80,  71,  230, 152, 225, 34,  248, 198, 63,  168, 179,
    141, 137, 5,   19,  79,  232, 128, 202, 46,  70,  37,  209, 217, 123, 27,
    177, 25,  56,  65,  229, 36,  197, 234, 108, 35,  151, 238, 200, 224, 99,
    190};

// Given the current hash value and a byte
// blobby_hash returns the new hash value
//
// Call repeatedly to hash a sequence of bytes, e.g.:
// uint8_t hash = 0;
// hash = blobby_hash(hash, byte0);
// hash = blobby_hash(hash, byte1);
// hash = blobby_hash(hash, byte2);
// ...

uint8_t blobby_hash(uint8_t hash, uint8_t byte) {
    return blobby_hash_table[hash ^ byte];
}
