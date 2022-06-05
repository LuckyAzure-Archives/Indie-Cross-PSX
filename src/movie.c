#include "stage.h"
#include "mem.h"
#include "loadscr.h"
#include "movie.h"
#include "font.h"
#include "main.h"
#include "menu.h"
#include "stage.h"
#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libgpu.h>
#include <libcd.h>
#include "strplay.c"

Movie movie;

STRFILE StrFile[] = {
	// File name	Resolution		Frame count
	{"\\VIDEOS\\C1.STR;1", 320, 240, 481,}
};

void PlayMovie() {
    // Reset and initialize stuff
	ResetCallback();
	CdInit();
	ResetGraph(0);
	SetGraphDebug(0);

}

void Movie_Tick(void) 
{
    Gfx_SetClear(0, 0, 0);
    switch (movie.select)
    {
        case 0:
			PlayMovie();
			PlayStr(320, 240, 0, 0, &StrFile[0]);
			break;
    }
}

