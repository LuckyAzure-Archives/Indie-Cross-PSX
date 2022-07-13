/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week1.h"

#include "../archive.h"
#include "../mem.h"
#include "../stage.h"

//Week 1 background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
	
	//Textures
	Gfx_Tex tex_back0; //Foreground
	Gfx_Tex tex_back1; //Middleground
	Gfx_Tex tex_back2; //Background
} Back_Week1;

//Week 1 background functions
void Back_Week1_DrawBG(StageBack *back)
{
	Back_Week1 *this = (Back_Week1*)back;
	
	fixed_t fx, fy;
	
	//Draw Foreground
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT foreground_src = {0, 0, 256, 144};
	RECT_FIXED foreground_dst = {
		FIXED_DEC(-250,1) - fx,
		FIXED_DEC(-250,1) - fy,
		FIXED_DEC(576,1),
		FIXED_DEC(324,1)
	};
	
	Stage_DrawTex(&this->tex_back0, &foreground_src, &foreground_dst, stage.camera.bzoom);

	fx = stage.camera.x / 2;
	fy = stage.camera.y / 2;

	//Draw Middleground
	RECT middleground_src = {0, 0, 256, 144};
	RECT_FIXED middleground_dst = {
		FIXED_DEC(-200,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(448,1),
		FIXED_DEC(252,1)
	};
	
	Stage_DrawTex(&this->tex_back1, &middleground_src, &middleground_dst, stage.camera.bzoom);

	fx = stage.camera.x / 6;
	fy = stage.camera.y / 6;

	//Draw Background
	RECT background_src = {0, 0, 256, 144};
	RECT_FIXED background_dst = {
		FIXED_DEC(-200,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(488,1),
		FIXED_DEC(252,1)
	};
	
	Stage_DrawTex(&this->tex_back2, &background_src, &background_dst, stage.camera.bzoom);
}

void Back_Week1_Free(StageBack *back)
{
	Back_Week1 *this = (Back_Week1*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Week1_New(void)
{
	//Allocate background structure
	Back_Week1 *this = (Back_Week1*)Mem_Alloc(sizeof(Back_Week1));
	if (this == NULL)
		return NULL;
	
	//Set background functions
	this->back.draw_fg = NULL;
	this->back.draw_md = NULL;
	this->back.draw_bg = Back_Week1_DrawBG;
	this->back.free = Back_Week1_Free;
	
	//Load background textures
	IO_Data arc_back = IO_Read("\\WEEK1\\BACK.ARC;1");
	Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
	Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
	Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
	Mem_Free(arc_back);
	
	return (StageBack*)this;
}
