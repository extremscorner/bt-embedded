#include "terminal.h"

bool quit_requested = false;

#ifdef __wii__

#include <fat.h>
#include <gccore.h>
#include <stdlib.h>

void on_reset_pressed(void)
{
    quit_requested = true;
}

void terminal_init(void)
{
    VIDEO_Init();
    GXRModeObj *rmode = VIDEO_GetPreferredMode(NULL);
    void *xfb = SYS_AllocateFramebuffer(rmode);
    CON_Init(xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitForFlush();
    fatInitDefault();

    SYS_SetResetCallback(on_reset_pressed);
}
#else

void terminal_init(void)
{
}

#endif
