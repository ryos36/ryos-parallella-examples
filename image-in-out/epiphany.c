#include "e_lib.h"
#include "shared_data.h"

//static uint8_t cache[16384];

int main(void)
{
    e_coreid_t coreid;
    uint32_t *startp, *endp;
    uint8_t *resultp;
    volatile uint32_t *done = (uint32_t *)(STATUS_ADDRESS + 8);
    volatile uint32_t *lock = (uint32_t *)(STATUS_ADDRESS + 12);

    unsigned int row, col, core, cores;

    coreid = e_get_coreid();
    e_coords_from_coreid(coreid, &row, &col);

    core = row * COLS + col;
    cores = ROWS * COLS;

    unsigned int frame = 1;

    volatile msg_block_t *msg = (msg_block_t *)COMMINUCATION_ADDRESS;
    volatile uint32_t *h2dp;

    unsigned int i = 0;
    while (1) {
        startp = (uint32_t *)(msg->msg[core].image_addr);
        endp = startp + 16384;
        resultp = (uint8_t *)(msg->msg[core].result_addr);

#define IMAGE_SIZE    0x00100000
        if ( startp != (uint32_t *)(IMAGE_ADDRESS + (512 * 512 * 4 / ( ROWS * COLS )) * core)) {
            goto error;
        }
        if ( resultp != (uint8_t *)(RESULT_ADDRESS + (512 * 512 / ( ROWS * COLS )) * core)) {
            goto error;
        }

        if ( startp == (uint32_t *)0x8e000000 ) {
            msg->msg[core].d2h |= 0x400;
        } else {
            //msg->msg[core].coreid = (uint32_t)startp + 0x1209;
        }
        if ( resultp == (uint8_t *)0x8e100000 ) {
            msg->msg[core].d2h |= 0x800;
        }

        msg->msg[core].coreid = coreid;
        h2dp = &msg->msg[core].h2d;

        __asm__ __volatile__ ("trap 4");
        while ( startp != endp ) {
            volatile uint32_t pix;
            float y0, y1, y2, y3, y4;
            uint16_t y16;

            pix = *startp;

            //0.257R + 0.504G + 0.098B
            y0 = 0.257 * ((pix & 0x00ff0000) >> 16);
            y1 = 0.504 * ((pix & 0x0000ff00) >>  8);
            y3 = y0 + y1;
            y2 = 0.098 * ((pix & 0x000000ff) >>  0);
            y4 = y2 + y3;
            y16 = (uint16_t)y4;
            *resultp = (uint8_t)y16;

            startp++;
            resultp++;

            *h2dp = 0;
            msg->msg[core].d2h |= 1;
        }
    
        *done |= 1 << core;
error:
        msg->msg[core].d2h = 4;
    }
    return 0;
}
