/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bfcup.h"

#include "../mem.h"
#include "../archive.h"
#include "../stage.h"
#include "../random.h"
#include "../main.h"

//Boyfriend skull fragments
static SkullFragment char_bfcup_skull[15] = {
	{ 1 * 8, -87 * 8, -13, -13},
	{ 9 * 8, -88 * 8,   5, -22},
	{18 * 8, -87 * 8,   9, -22},
	{26 * 8, -85 * 8,  13, -13},
	
	{-3 * 8, -82 * 8, -13, -11},
	{ 8 * 8, -85 * 8,  -9, -15},
	{20 * 8, -82 * 8,   9, -15},
	{30 * 8, -79 * 8,  13, -11},
	
	{-1 * 8, -74 * 8, -13, -5},
	{ 8 * 8, -77 * 8,  -9, -9},
	{19 * 8, -75 * 8,   9, -9},
	{26 * 8, -74 * 8,  13, -5},
	
	{ 5 * 8, -73 * 8, -5, -3},
	{14 * 8, -76 * 8,  9, -6},
	{26 * 8, -67 * 8, 15, -3},
};

//Boyfriend player types
enum
{
	BFCup_ArcMain_BFCup0,
	BFCup_ArcMain_BFCup1,
	BFCup_ArcMain_BFCup2,
	BFCup_ArcMain_BFCup3,
	BFCup_ArcMain_BFCup4,
	BFCup_ArcMain_BFCup5,
	BFCup_ArcMain_BFCup6,
	BFCup_ArcMain_Dead0, //BREAK
	BFCup_ArcDead_Dead1, //Mic Drop
	BFCup_ArcDead_Dead2, //Twitch
	BFCup_ArcDead_Retry,
	
	BFCup_ArcMain_Max,
};

#define BFCup_Arc_Max BFCup_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[BFCup_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
	
	u8 retry_bump;
	
	SkullFragment skull[COUNT_OF(char_bfcup_skull)];
	u8 skull_scale;
} Char_BFCup;

//Boyfriend player definitions
static const CharFrame char_bfcup_frame[] = {
	{BFCup_ArcMain_BFCup0, {  1,   4, 110,  98}, {110,  98}}, //0 idle 1
	{BFCup_ArcMain_BFCup0, {114,   4, 104, 101}, {104, 101}}, //1 idle 2
	{BFCup_ArcMain_BFCup0, {  1, 114, 104, 102}, {104, 102}}, //2 idle 3
	{BFCup_ArcMain_BFCup0, {112, 114, 103, 103}, {103, 103}}, //3 idle 4
	{BFCup_ArcMain_BFCup1, {  2,   1, 102, 103}, {102, 103}}, //4 idle 5
	
	{BFCup_ArcMain_BFCup1, {106,   7, 111,  98}, {120,  98}}, //5 left 1
	{BFCup_ArcMain_BFCup1, {  2, 112,  96, 103}, {105, 103}}, //6 left 2
	{BFCup_ArcMain_BFCup1, {115, 111,  96, 103}, {102, 103}}, //7 left 3
	{BFCup_ArcMain_BFCup2, {  1,   3,  95, 103}, {100, 103}}, //8 left 4
	
	{BFCup_ArcMain_BFCup2, {108,   2,  98,  88}, {104,  88}}, //9 down 1
	{BFCup_ArcMain_BFCup2, {  1, 116,  94,  93}, {102,  92}}, //10 down 2
	{BFCup_ArcMain_BFCup2, {112,  99,  94,  93}, {102,  93}}, //11 down 3
	{BFCup_ArcMain_BFCup3, {  1,   4,  95,  94}, {102,  92}}, //12 down 4
	
	{BFCup_ArcMain_BFCup3, {119,   2,  89, 119}, { 89, 118}}, //13 up 1
	{BFCup_ArcMain_BFCup3, {  4, 108,  96, 109}, { 95, 109}}, //14 up 2
	{BFCup_ArcMain_BFCup3, {111, 128,  98, 108}, { 95, 108}}, //15 up 3
	
	{BFCup_ArcMain_BFCup4, {  1,   9, 116,  98}, { 96,  98}}, //16 right 1
	{BFCup_ArcMain_BFCup4, {118,   3, 100, 103}, { 95, 103}}, //17 right 2
	{BFCup_ArcMain_BFCup4, {  2, 115,  99, 103}, { 98, 103}}, //18 right 3
	{BFCup_ArcMain_BFCup4, {111, 115,  99, 103}, {100, 103}}, //19 right 4
	
	{BFCup_ArcMain_BFCup5, {  0,   0,  93, 108}, {102, 107}}, //20 left miss 1
	{BFCup_ArcMain_BFCup5, { 94,   0,  93, 108}, {102, 107}}, //21 left miss 2
	
	{BFCup_ArcMain_BFCup5, {  0, 109,  95,  98}, {102,  96}}, //22 down miss 1
	{BFCup_ArcMain_BFCup5, { 96, 109,  95,  97}, {102,  95}}, //23 down miss 2
	
	{BFCup_ArcMain_BFCup6, {  0,   0,  90, 107}, { 95, 108}}, //24 up miss 1
	{BFCup_ArcMain_BFCup6, { 91,   0,  89, 108}, { 95, 109}}, //25 up miss 2
	
	{BFCup_ArcMain_BFCup6, {  0, 108,  99, 108}, { 92, 108}}, //26 right miss 1
	{BFCup_ArcMain_BFCup6, {100, 109, 101, 108}, { 93, 108}}, //27 right miss 2

	{BFCup_ArcMain_Dead0, {  0,   0, 128, 128}, { 53,  98}}, //23 dead0 0
	{BFCup_ArcMain_Dead0, {128,   0, 128, 128}, { 53,  98}}, //24 dead0 1
	{BFCup_ArcMain_Dead0, {  0, 128, 128, 128}, { 53,  98}}, //25 dead0 2
	{BFCup_ArcMain_Dead0, {128, 128, 128, 128}, { 53,  98}}, //26 dead0 3
	
	{BFCup_ArcDead_Dead1, {  0,   0, 128, 128}, { 53,  98}}, //27 dead1 0
	{BFCup_ArcDead_Dead1, {128,   0, 128, 128}, { 53,  98}}, //28 dead1 1
	{BFCup_ArcDead_Dead1, {  0, 128, 128, 128}, { 53,  98}}, //29 dead1 2
	{BFCup_ArcDead_Dead1, {128, 128, 128, 128}, { 53,  98}}, //30 dead1 3
	
	{BFCup_ArcDead_Dead2, {  0,   0, 128, 128}, { 53,  98}}, //31 dead2 body twitch 0
	{BFCup_ArcDead_Dead2, {128,   0, 128, 128}, { 53,  98}}, //32 dead2 body twitch 1
	{BFCup_ArcDead_Dead2, {  0, 128, 128, 128}, { 53,  98}}, //33 dead2 balls twitch 0
	{BFCup_ArcDead_Dead2, {128, 128, 128, 128}, { 53,  98}}, //34 dead2 balls twitch 1
};

static const Animation char_bfcup_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6,  7,  8, ASCR_BACK, 1}},     //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 9, 10, 11, 12, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){16, 17, 18, 19, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 5, 20, 20, 21, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 9, 22, 22, 23, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){13, 24, 24, 25, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){16, 26, 26, 27, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},         //PlayerAnim_Peace
	{2, (const u8[]){16, 17, 18, 19, ASCR_REPEAT}},      //PlayerAnim_Sweat
	
	{3, (const u8[]){28, 29, 30, 31, 31, 31, 31, 31, 31, 31, ASCR_CHGANI, PlayerAnim_Dead1}}, //PlayerAnim_Dead0
	{2, (const u8[]){31, ASCR_REPEAT}},                                                       //PlayerAnim_Dead1
	{3, (const u8[]){32, 33, 34, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}}, //PlayerAnim_Dead2
	{2, (const u8[]){35, ASCR_REPEAT}},                                                       //PlayerAnim_Dead3
	{3, (const u8[]){36, 37, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead4
	{3, (const u8[]){38, 39, 35, 35, 35, 35, 35, ASCR_CHGANI, PlayerAnim_Dead3}},             //PlayerAnim_Dead5
	
	{10, (const u8[]){35, 35, 35, ASCR_BACK, 1}}, //PlayerAnim_Dead4
	{ 3, (const u8[]){38, 39, 35, ASCR_REPEAT}},  //PlayerAnim_Dead5
};

//Boyfriend player functions
void Char_BFCup_SetFrame(void *user, u8 frame)
{
	Char_BFCup *this = (Char_BFCup*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfcup_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BFCup_Tick(Character *character)
{
	Char_BFCup *this = (Char_BFCup*)character;
	
	//Handle animation updates
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
	}
	
	//Retry screen
	if (character->animatable.anim >= PlayerAnim_Dead3)
	{
		//Tick skull fragments
		if (this->skull_scale)
		{
			SkullFragment *frag = this->skull;
			for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BFCup, skull); i++, frag++)
			{
				//Draw fragment
				RECT frag_src = {
					(i & 1) ? 112 : 96,
					(i >> 1) << 4,
					16,
					16
				};
				fixed_t skull_dim = (FIXED_DEC(16,1) * this->skull_scale) >> 6;
				fixed_t skull_rad = skull_dim >> 1;
				RECT_FIXED frag_dst = {
					character->x + (((fixed_t)frag->x << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.x,
					character->y + (((fixed_t)frag->y << FIXED_SHIFT) >> 3) - skull_rad - stage.camera.y,
					skull_dim,
					skull_dim,
				};
				Stage_DrawTex(&this->tex_retry, &frag_src, &frag_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
				
				//Move fragment
				frag->x += frag->xsp;
				frag->y += ++frag->ysp;
			}
			
			//Decrease scale
			this->skull_scale--;
		}
		
		//Draw input options
		u8 input_scale = 16 - this->skull_scale;
		if (input_scale > 16)
			input_scale = 0;
		
		RECT button_src = {
			 0, 96,
			16, 16
		};
		RECT_FIXED button_dst = {
			character->x - FIXED_DEC(32,1) - stage.camera.x,
			character->y - FIXED_DEC(88,1) - stage.camera.y,
			(FIXED_DEC(16,1) * input_scale) >> 4,
			FIXED_DEC(16,1),
		};
		
		//Cross - Retry
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Circle - Blueball
		button_src.x = 16;
		button_dst.y += FIXED_DEC(56,1);
		Stage_DrawTex(&this->tex_retry, &button_src, &button_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
		
		//Draw 'RETRY'
		u8 retry_frame;
		
		if (character->animatable.anim == PlayerAnim_Dead6)
		{
			//Selected retry
			retry_frame = 2 - (this->retry_bump >> 3);
			if (retry_frame >= 3)
				retry_frame = 0;
			if (this->retry_bump & 2)
				retry_frame += 3;
			
			if (++this->retry_bump == 0xFF)
				this->retry_bump = 0xFD;
		}
		else
		{
			//Idle
			retry_frame = 1 +  (this->retry_bump >> 2);
			if (retry_frame >= 3)
				retry_frame = 0;
			
			if (++this->retry_bump >= 55)
				this->retry_bump = 0;
		}
		
		RECT retry_src = {
			(retry_frame & 1) ? 48 : 0,
			(retry_frame >> 1) << 5,
			48,
			32
		};
		RECT_FIXED retry_dst = {
			character->x -  FIXED_DEC(7,1) - stage.camera.x,
			character->y - FIXED_DEC(92,1) - stage.camera.y,
			FIXED_DEC(48,1),
			FIXED_DEC(32,1),
		};
		Stage_DrawTex(&this->tex_retry, &retry_src, &retry_dst, FIXED_MUL(stage.camera.zoom, stage.bump));
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BFCup_SetFrame);
	Character_Draw(character, &this->tex, &char_bfcup_frame[this->frame]);
}

void Char_BFCup_SetAnim(Character *character, u8 anim)
{
	Char_BFCup *this = (Char_BFCup*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_Dead0:
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_Dead2:
			//Load retry art
			Gfx_LoadTex(&this->tex_retry, this->arc_ptr[BFCup_ArcDead_Retry], 0);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BFCup_Free(Character *character)
{
	Char_BFCup *this = (Char_BFCup*)character;
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_BFCup_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BFCup *this = Mem_Alloc(sizeof(Char_BFCup));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFCup_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFCup_Tick;
	this->character.set_anim = Char_BFCup_SetAnim;
	this->character.free = Char_BFCup_Free;
	
	Animatable_Init(&this->character.animatable, char_bfcup_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	this->character.health_i = 0;
	
	this->character.focus_x = FIXED_DEC(-30,1);
	this->character.focus_y = (stage.stage_id == StageId_1_3) ? FIXED_DEC(-95,1) : FIXED_DEC(-95,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	this->character.size = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BFCUP.ARC;1");
	this->arc_dead = NULL;
	
	const char **pathp = (const char *[]){
		"bf0.tim",   //BFCup_ArcMain_BFCup0
		"bf1.tim",   //BFCup_ArcMain_BFCup1
		"bf2.tim",   //BFCup_ArcMain_BFCup2
		"bf3.tim",   //BFCup_ArcMain_BFCup3
		"bf4.tim",   //BFCup_ArcMain_BFCup4
		"bf5.tim",   //BFCup_ArcMain_BFCup5
		"bf6.tim",   //BFCup_ArcMain_BFCup6
		"dead0.tim", //BFCup_ArcMain_Dead0
		"dead1.tim", //BFCup_ArcDead_Dead1
		"dead2.tim", //BFCup_ArcDead_Dead2
		"retry.tim", //BFCup_ArcDead_Retry
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	//Initialize player state
	this->retry_bump = 0;
	
	//Copy skull fragments
	memcpy(this->skull, char_bfcup_skull, sizeof(char_bfcup_skull));
	this->skull_scale = 64;
	
	SkullFragment *frag = this->skull;
	for (size_t i = 0; i < COUNT_OF_MEMBER(Char_BFCup, skull); i++, frag++)
	{
		//Randomize trajectory
		frag->xsp += RandomRange(-4, 4);
		frag->ysp += RandomRange(-2, 2);
	}
	
	return (Character*)this;
}
