#include <stdint.h>

#define ROWS 4
#define COLS 4

#define IMAGE_ADDRESS 0x8e000000
#define RESULT_ADDRESS 0x8e100000
#define COMMINUCATION_ADDRESS 0x8f001000
#define STATUS_ADDRESS 0x8f000000

#define MAX_CORE_N 16
#define ALIGN8 8

typedef struct __attribute__((aligned(ALIGN8)))
{
    uint32_t status;
    uint32_t errno;
    uint32_t done;

    uint32_t lock;
} status_t;

typedef struct __attribute__((aligned(ALIGN8)))
{
    uint32_t d2h;
    uint32_t coreid;

    uint32_t h2d;
    uint32_t image_addr;
    uint32_t result_addr;
} msg_t;

typedef struct
{
    msg_t msg[MAX_CORE_N];
} msg_block_t;

