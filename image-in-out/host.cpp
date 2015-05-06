#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <e-hal.h>
#include <IL/il.h>
#include "shared_data.h"

#define IMAGE_OFFSET  0x00000000
#define IMAGE_SIZE    0x00100000
#define RESULT_OFFSET 0x00100000
#define RESULT_SIZE   0x00040000

#define STATUS_OFFSET    0x01000000
#define COMMUNICATION_OFFSET    0x01001000

static inline void nano_wait(uint32_t sec, uint32_t nsec)
{
    struct timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = nsec;
    nanosleep(&ts, NULL);
}

char image[512*512*4]; // for lenna

int main(int argc, char *argv[])
{
    e_platform_t eplat;
    e_epiphany_t edev;
    status_t status;

    e_mem_t status_emem;
    e_mem_t comm_emem;
    e_mem_t image_emem;
    e_mem_t result_emem;

    msg_block_t msg;
    memset(&msg, 0, sizeof(msg));

    struct timespec time;
    double time0, time1;

    e_init(NULL);
    e_reset_system();
    e_get_platform_info(&eplat);

    e_alloc(&status_emem, STATUS_OFFSET, sizeof(msg_block_t));
    e_alloc(&comm_emem, COMMUNICATION_OFFSET, sizeof(msg_block_t));
    e_alloc(&image_emem, IMAGE_OFFSET, IMAGE_SIZE);
    e_alloc(&result_emem, RESULT_OFFSET, RESULT_SIZE);

    memset(&status, 0, sizeof(status_t));
    e_write(&status_emem, 0, 0, 0, &status, sizeof(status_t));
    if ( 0 ) {
        ILuint  ImgId;
        ILubyte *imdata;
        ILuint  imsize, imBpp;
        ilInit();
        //ilApplyProfile(0, (char *)"RGB32");
        ilGenImages(1, &ImgId);
        ilBindImage(ImgId);
        if (!ilLoadImage(argv[1])) {
            fprintf(stderr, "error\n");
            exit(3);
        }
        printf("Width: %d  Height: %d  Depth: %d  Bpp: %d BytesPP: %d \n\n",
               ilGetInteger(IL_IMAGE_WIDTH),
               ilGetInteger(IL_IMAGE_HEIGHT),
               ilGetInteger(IL_IMAGE_DEPTH),
               ilGetInteger(IL_IMAGE_BITS_PER_PIXEL),
               ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL));


        imdata = ilGetData();
        imsize = ilGetInteger(IL_IMAGE_WIDTH) * ilGetInteger(IL_IMAGE_HEIGHT);
        imBpp  = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

        for( int j = 0 ; j < 512 * 512; ++j ) {
            image[j * 4 + 0] = 0;
            image[j * 4 + 1] = image[j * 3 + 0];
            image[j * 4 + 2] = image[j * 3 + 1];
            image[j * 4 + 3] = image[j * 3 + 2];
        }
        e_write(&image_emem, 0, 0, 0, imdata, imsize);
    }

    FILE *inf;
    inf = fopen("lenna1.raw", "rb");
    if ( inf ) {
        fread(image, 512 * 512, 4, inf);
        fclose(inf);
        e_write(&image_emem, 0, 0, 0, image, sizeof(image));
    } else {
        fprintf(stderr, "open error\n");
    }

    unsigned int row = 0;
    unsigned int col = 0;

    volatile unsigned int vcoreid = 0;

    e_open(&edev, 0, 0, ROWS, COLS);
    e_write(&comm_emem, 0, 0, 0, &msg, sizeof(msg)); // zero clear

    uint32_t image_addr;
    uint32_t result_addr;

    image_addr = IMAGE_ADDRESS + IMAGE_OFFSET;
    result_addr = IMAGE_ADDRESS + RESULT_OFFSET;

    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            unsigned int core = row * COLS + col;
            msg.msg[core].image_addr = image_addr;
            msg.msg[core].result_addr = result_addr;

            image_addr += (512 * 512 * 4 / ( ROWS * COLS ));
            result_addr += (512 * 512 / ( ROWS * COLS ));
        }
    }
    e_write(&comm_emem, 0, 0, 0, &msg, sizeof(msg)); // set image_addr result_addr

    e_reset_group(&edev);
    e_load_group((char *)"epiphany.srec", &edev, 0, 0, ROWS, COLS, E_TRUE);

    nano_wait(0, 100000000);
    clock_gettime(CLOCK_REALTIME, &time);
    time0 = time.tv_sec + time.tv_nsec * 1.0e-9;

    while (true) {
        for (row = 0; row < ROWS; row++) {
            for (col = 0; col < COLS; col++) {
                e_resume(&edev, row, col);
            }
        }
        int x;
        for (row = 0; row < ROWS; row++) {
            for (col = 0; col < COLS; col++) {
                unsigned int core = row * COLS + col;

                x = 0;
                while (true) {
                    e_read(&comm_emem, 0, 0, (off_t)((char *)&msg.msg[core] - (char *)&msg), &msg.msg[core], sizeof(uint32_t) * 2);
                    e_read(&comm_emem, 0, 0, (off_t)((char *)&msg.msg[core].h2d - (char *)&msg), &msg.msg[core].h2d, sizeof(uint32_t));
                    vcoreid = msg.msg[core].coreid;
                    
#if 0
                    if ( msg.msg[core].h2d == 0 ) {
                        //printf("%d %d %d %x row:%d col:%d\n", msg.msg[core].d2h, msg.msg[core].h2d, x++, vcoreid, (vcoreid >> 6), (vcoreid & 0x3f));
                    }
                    if (( msg.msg[core].d2h & 1 ) == 1 ) {
                        msg.msg[core].h2d = 1;
                        e_write(&comm_emem, 0, 0, (off_t)((char *)&msg.msg[core].h2d - (char *)&msg), &msg.msg[core].h2d, sizeof(uint32_t));
                    }
#endif
                    
                    if (( msg.msg[core].d2h & 4 ) == 4 ) {
                        break;
                    }
                    //nano_wait(0, 1000000);
                }
                vcoreid = msg.msg[core].coreid;
                printf("row:%d col:%d\n", (vcoreid >> 6), (vcoreid & 0x3f));
                //e_write(&comm_emem, 0, 0, (off_t)((char *)&msg.msg_h2d[core] - (char *)&msg), &msg.msg_h2d[core], sizeof(msg_host2dev_t));
            }
        }
        break;
    }
    e_read(&result_emem, 0, 0, 0, image, 512 * 512);
    FILE *f;
    f = fopen("result.raw", "wb");
    if ( f ) {
        fwrite(image, 512 * 512, 1, f);
        fclose(f);
    } else {
        fprintf(stderr, "open error\n");
    }

    clock_gettime(CLOCK_REALTIME, &time);
    time1 = time.tv_sec + time.tv_nsec * 1.0e-9;
    printf("time: %f sec\n", time1 - time0);

    e_read(&status_emem, 0, 0, 0, &status, sizeof(status_t));
    printf("status done:%x\n", status.done);

    e_close(&edev);
    e_free(&comm_emem);
    e_free(&status_emem);
    e_free(&image_emem);
    e_free(&result_emem);
    e_finalize();
    return 0;
}
