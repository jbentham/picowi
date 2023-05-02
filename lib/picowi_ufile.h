// Definitions for UFILE file transfers

#define UFILE_PORTNUM   40004
#define MAX_BLOCKLEN    1400
#define MAX_REQUESTS    8

#define REQ_ID          "UFR1"
#define RESP_ID         "ufr1"
#define START_BLOCKNUM  1

#pragma pack(1)
typedef struct          // Resource ident block..
{
    char pcol[4];       // Protocol identification
    char fname[16];     // Resource name
    int flen;           // Total length (-1 if streaming)
    int blocklen;       // Length of 1 block
    int blocknums[1];   // Block nums (1+ blocks for request, 1 for response)
} UFILE_HDR;

typedef struct
{
    UFILE_HDR hdr;
    unsigned char data[MAX_BLOCKLEN - sizeof(UFILE_HDR)];
} UFILE_MSG;
#pragma pack()

// EOF
