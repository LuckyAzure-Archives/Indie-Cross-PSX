/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "menu.h"

#include "mem.h"
#include "main.h"
#include "timer.h"
#include "io.h"
#include "gfx.h"
#include "audio.h"
#include "pad.h"
#include "archive.h"
#include "mutil.h"

#include "movie.h"

#include "font.h"
#include "trans.h"
#include "loadscr.h"

#include "stage.h"
#include "character/gf.h"

//Menu messages
static const char *funny_messages[][2] = {
	{"PSX PORT BY CUCKYDEV", "YOU KNOW IT"},
	{"PORTED BY CUCKYDEV", "WHAT YOU GONNA DO"},
	{"FUNKIN", "FOREVER"},
	{"WHAT THE HELL", "RITZ PSX"},
	{"LIKE PARAPPA", "BUT COOLER"},
	{"THE JAPI", "EL JAPI"},
	{"PICO FUNNY", "PICO FUNNY"},
	{"OPENGL BACKEND", "BY CLOWNACY"},
	{"CUCKYFNF", "SETTING STANDARDS"},
	{"lool", "inverted colours"},
	{"NEVER LOOK AT", "THE ISSUE TRACKER"},
	{"PSXDEV", "HOMEBREW"},
	{"ZERO POINT ZERO TWO TWO EIGHT", "ONE FIVE NINE ONE ZERO FIVE"},
	{"DOPE ASS GAME", "PLAYSTATION MAGAZINE"},
	{"NEWGROUNDS", "FOREVER"},
	{"NO FPU", "NO PROBLEM"},
	{"OK OKAY", "WATCH THIS"},
	{"ITS MORE MALICIOUS", "THAN ANYTHING"},
	{"USE A CONTROLLER", "LOL"},
	{"SNIPING THE KICKSTARTER", "HAHA"},
	{"SHITS UNOFFICIAL", "NOT A PROBLEM"},
	{"SYSCLK", "RANDOM SEED"},
	{"THEY DIDNT HIT THE GOAL", "STOP"},
	{"FCEFUWEFUETWHCFUEZDSLVNSP", "PQRYQWENQWKBVZLZSLDNSVPBM"},
	{"THE FLOORS ARE", "THREE DIMENSIONAL"},
	{"PSXFUNKIN BY CUCKYDEV", "SUCK IT DOWN"},
	{"PLAYING ON EPSXE HUH", "YOURE THE PROBLEM"},
	{"NEXT IN LINE", "ATARI"},
	{"HAXEFLIXEL", "COME ON"},
	{"HAHAHA", "I DONT CARE"},
	{"GET ME TO STOP", "TRY"},
	{"FNF MUKBANG GIF", "THATS UNRULY"},
	{"OPEN SOURCE", "FOREVER"},
	{"ITS A PORT", "ITS WORSE"},
	{"WOW GATO", "WOW GATO"},
	{"BALLS FISH", "BALLS FISH"},
};

//Menu string type
#define MENUSTR_CHARS 0x20
typedef char MenuStr[MENUSTR_CHARS + 1];

//Menu state
static struct
{
	//Menu state
	u8 page, next_page;
	boolean page_swap;
	u8 select, next_select;
	
	fixed_t scroll;
	fixed_t trans_time;
	
	//Page specific state
	union
	{
		struct
		{
			u8 funny_message;
		} opening;
		struct
		{
			fixed_t logo_bump;
			fixed_t fade, fadespd;
			s8 animcounter;
		} title;
		struct
		{
			fixed_t fade, fadespd;
		} story;
		struct
		{
			fixed_t back_r, back_g, back_b;
		} freeplay;
	} page_state;
	
	union
	{
		struct
		{
			u8 id, diff;
			boolean story;
		} stage;
	} page_param;
	
	//Menu assets
	Gfx_Tex tex_back, tex_ng, tex_story, tex_title;
	FontData font_bold, font_arial;
	
	Character *gf; //Title Girlfriend
} menu;


//Internal menu functions
char menu_text_buffer[0x100];

static const char *Menu_LowerIf(const char *text, boolean lower)
{
	//Copy text
	char *dstp = menu_text_buffer;
	if (lower)
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'A' && *srcp <= 'Z')
				*dstp++ = *srcp | 0x20;
			else
				*dstp++ = *srcp;
		}
	}
	else
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'a' && *srcp <= 'z')
				*dstp++ = *srcp & ~0x20;
			else
				*dstp++ = *srcp;
		}
	}
	
	//Terminate text
	*dstp++ = '\0';
	return menu_text_buffer;
}

static void Menu_DrawBack(s8 type)
{
	RECT back_dst = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	RECT back_src = {0, 0, 255, 255};
	
	switch (type)
	{
		case 0:
			back_src.x = 254;
			back_src.w = 1;
			Gfx_DrawTex(&menu.tex_title, &back_src, &back_dst);
		case 1:
			RECT back_src = {0, 0, 255, 255};
			Gfx_DrawTex(&menu.tex_back, &back_src, &back_dst);
	}
}

static void Menu_DifficultySelector(s32 x, s32 y)
{
	//Change difficulty
	if (menu.next_page == menu.page && Trans_Idle())
	{
		if (pad_state.press & PAD_LEFT)
		{
			if (menu.page_param.stage.diff > StageDiff_Easy)
				menu.page_param.stage.diff--;
			else
				menu.page_param.stage.diff = StageDiff_Hard;
		}
		if (pad_state.press & PAD_RIGHT)
		{
			if (menu.page_param.stage.diff < StageDiff_Hard)
				menu.page_param.stage.diff++;
			else
				menu.page_param.stage.diff = StageDiff_Easy;
		}
	}
	
	//Draw difficulty arrows
	static const RECT arrow_src[2][2] = {
		{{224, 64, 16, 32}, {224, 96, 16, 32}}, //left
		{{240, 64, 16, 32}, {240, 96, 16, 32}}, //right
	};
	
	Gfx_BlitTex(&menu.tex_story, &arrow_src[0][(pad_state.held & PAD_LEFT) != 0], x - 40 - 16, y - 16);
	Gfx_BlitTex(&menu.tex_story, &arrow_src[1][(pad_state.held & PAD_RIGHT) != 0], x + 40, y - 16);
	
	//Draw difficulty
	static const RECT diff_srcs[] = {
		{  0, 96, 64, 18},
		{ 64, 96, 80, 18},
		{144, 96, 64, 18},
	};
	
	const RECT *diff_src = &diff_srcs[menu.page_param.stage.diff];
	Gfx_BlitTex(&menu.tex_story, diff_src, x - (diff_src->w >> 1), y - 9 + ((pad_state.press & (PAD_LEFT | PAD_RIGHT)) != 0));
}

static void Menu_DrawWeek(const char *week, s32 x, s32 y)
{
	//Draw label
	if (week == NULL)
	{
		//Tutorial
		RECT label_src = {0, 0, 112, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
	}
	else
	{
		//Week
		RECT label_src = {0, 32, 80, 32};
		Gfx_BlitTex(&menu.tex_story, &label_src, x, y);
		
		//Number
		x += 80;
		for (; *week != '\0'; week++)
		{
			//Draw number
			u8 i = *week - '0';
			
			RECT num_src = {128 + ((i & 3) << 5), ((i >> 2) << 5), 32, 32};
			Gfx_BlitTex(&menu.tex_story, &num_src, x, y);
			x += 32;
		}
	}
}

//Menu functions
void Menu_Load(MenuPage page)
{
	//Load menu assets
	IO_Data menu_arc = IO_Read("\\MENU\\MENU.ARC;1");
	Gfx_LoadTex(&menu.tex_back,  Archive_Find(menu_arc, "back.tim"),  0);
	Gfx_LoadTex(&menu.tex_ng,    Archive_Find(menu_arc, "ng.tim"),    0);
	Gfx_LoadTex(&menu.tex_story, Archive_Find(menu_arc, "story.tim"), 0);
	Gfx_LoadTex(&menu.tex_title, Archive_Find(menu_arc, "title.tim"), 0);
	Mem_Free(menu_arc);
	
	FontData_Load(&menu.font_bold, Font_Bold);
	FontData_Load(&menu.font_arial, Font_Arial);
	
	menu.gf = Char_GF_New(FIXED_DEC(62,1), FIXED_DEC(-12,1));
	stage.camera.x = stage.camera.y = FIXED_DEC(0,1);
	stage.camera.bzoom = FIXED_UNIT;
	stage.gf_speed = 4;
	
	//Initialize menu state
	menu.select = menu.next_select = 0;
	
	switch (menu.page = menu.next_page = page)
	{
		case MenuPage_Opening:
			//Get funny message to use
			//Do this here so timing is less reliant on VSync
			#ifdef PSXF_PC
				menu.page_state.opening.funny_message = time(NULL) % COUNT_OF(funny_messages);
			#else
				menu.page_state.opening.funny_message = ((*((volatile u32*)0xBF801120)) >> 3) % COUNT_OF(funny_messages); //sysclk seeding
			#endif
			break;
		default:
			break;
	}
	menu.page_swap = true;
	
	menu.trans_time = 0;
	Trans_Clear();
	
	stage.song_step = 0;
	
	//Play menu music
	Audio_LoadMus("\\MUSIC\\MENU.MUS;1");
	Audio_PlayMus(true);
	Audio_SetVolume(0, 0x3FFF, 0x0000);
	Audio_SetVolume(1, 0x0000, 0x3FFF);
	
	//Set background colour
	Gfx_SetClear(0, 0, 0);
}

void Menu_Unload(void)
{
	//Free title Girlfriend
	Character_Free(menu.gf);
}

void Menu_ToStage(StageId id, StageDiff diff, boolean story)
{
	menu.next_page = MenuPage_Stage;
	menu.page_param.stage.id = id;
	menu.page_param.stage.story = story;
	menu.page_param.stage.diff = diff;
	Trans_Start();
}

void Menu_Tick(void)
{
	//Clear per-frame flags
	stage.flag &= ~STAGE_FLAG_JUST_STEP;
	
	//Get song position
	u16 next_step = Audio_GetTime() / FIXED_DEC(15, 102);
	if (next_step != stage.song_step)
	{
		if (next_step >= stage.song_step)
			stage.flag |= STAGE_FLAG_JUST_STEP;
		stage.song_step = next_step;
	}
	
	//Handle transition out
	if (Trans_Tick())
	{
		//Change to set next page
		menu.page_swap = true;
		menu.page = menu.next_page;
		menu.select = menu.next_select;
	}
	
	//Tick menu page
	MenuPage exec_page;
	switch (exec_page = menu.page)
	{
	//Fallthrough
		case MenuPage_Title:
		{
			if (pad_state.press & PAD_TRIANGLE)
				{
				stage.widescreen = true;
				stage.ghost = true;
				stage.downscroll = true;
				}
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(90,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Go to main menu when start is pressed
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if ((pad_state.press & PAD_START) && menu.next_page == menu.page && Trans_Idle())
			{
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
				menu.next_page = MenuPage_Main;
				menu.next_select = 0;
			}
			
			//Draw Friday Night Funkin' logo
			if ((stage.flag & STAGE_FLAG_JUST_STEP) && (stage.song_step & 0x3) == 0 && menu.page_state.title.logo_bump == 0)
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
			
			static const fixed_t logo_scales[] = {
				FIXED_DEC(100,100),
				FIXED_DEC(101,100),
				FIXED_DEC(102,100),
				FIXED_DEC(103,100),
				FIXED_DEC(105,100),
				FIXED_DEC(110,100),
				FIXED_DEC(97,100),
			};
			fixed_t logo_scale = logo_scales[(menu.page_state.title.logo_bump * 24) >> FIXED_SHIFT];
			u32 x_rad = (logo_scale * (179 >> 1)) >> FIXED_SHIFT;
			u32 y_rad = (logo_scale * (114 >> 1)) >> FIXED_SHIFT;
			u32 x_rad2 = (logo_scale * (115 >> 1)) >> FIXED_SHIFT;
			u32 y_rad2 = (logo_scale * (8 >> 1)) >> FIXED_SHIFT;
			
			RECT logo2_src = {1, 115, 115, 8};
			RECT logo2_dst = {
				100 + (SCREEN_WIDEADD2 >> 1),
				140,
				x_rad2 << 1,
				y_rad2 << 1
			};
			
			Gfx_DrawTexRotate(&menu.tex_title, &logo2_src, &logo2_dst, -2);
			
			RECT logo_src = {0, 0, 179, 114};
			RECT logo_dst = {
				100 + (SCREEN_WIDEADD2 >> 1),
				76,
				x_rad << 1,
				y_rad << 1
			};
			
			Gfx_DrawTexRotate(&menu.tex_title, &logo_src, &logo_dst, -2);
			
			if (menu.page_state.title.logo_bump > 0)
				if ((menu.page_state.title.logo_bump -= timer_dt) < 0)
					menu.page_state.title.logo_bump = 0;
			
			//Draw "Press Start to Begin"
			if (menu.next_page == menu.page)
			{
				//Blinking blue
				s16 press_lerp = (MUtil_Cos(animf_count << 3) + 0x100) >> 1;
				u8 press_r = 51 >> 1;
				u8 press_g = (58  + ((press_lerp * (255 - 58))  >> 8)) >> 1;
				u8 press_b = (206 + ((press_lerp * (255 - 206)) >> 8)) >> 1;
				
				RECT press_src = {1, 124, 89, 30};
				Gfx_BlitTexCol(&menu.tex_title, &press_src, 200, SCREEN_HEIGHT - 48, press_r, press_g, press_b);
			}
			else
			{
				//Flash white
				RECT press_src = {1, 124, 89, 30};
				Gfx_BlitTex(&menu.tex_title, &press_src, 200, SCREEN_HEIGHT - 48);
			}
			
			menu.page_state.title.animcounter += 1;
			if (menu.page_state.title.animcounter > 50)
				menu.page_state.title.animcounter = 0;
			
			RECT effect_src = {91, 124, 17, 30};
			RECT effect_dst = {
				272 + menu.page_state.title.animcounter,
				SCREEN_HEIGHT - 48,
				17,
				30
			};
			
			Gfx_BlendTex(&menu.tex_title, &effect_src, &effect_dst, 2);
			
			//Draw Girlfriend
			menu.gf->tick(menu.gf);
			
			//Draw background
			Menu_DrawBack(0);
			
			break;
		}
		case MenuPage_Main:
		{
			static const char *menu_options[] = {
				"STORY MODE",
				"FREEPLAY",
				"CUSTOM SONGS",
				"OPTIONS",
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = menu.select *
				FIXED_DEC(12,1);
			
			//Draw version identification
			menu.font_bold.draw(&menu.font_bold,
				"PSXFUNKIN BY CUCKYDEV",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					switch (menu.select)
					{
						case 0: //Story Mode
							menu.next_page = MenuPage_Story;
							break;
						case 1: //Freeplay
							menu.next_page = MenuPage_Freeplay;
							break;
						case 2: //Mods
							menu.next_page = MenuPage_Mods;
							break;
						case 3: //Options
							menu.next_page = MenuPage_Options;
							break;
					}
					menu.next_select = 0;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to title screen if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					menu.next_page = MenuPage_Title;
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select *
			FIXED_DEC(12,1);
			menu.scroll += (next_scroll - menu.scroll) >> 2;
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Title)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					menu.font_bold.draw(&menu.font_bold,
						Menu_LowerIf(menu_options[i], menu.select != i),
						SCREEN_WIDTH2,
						SCREEN_HEIGHT2 + (i << 5) - 48 - (menu.scroll >> FIXED_SHIFT),
						FontAlign_Center
					);
				}
			}
			else if (animf_count & 2)
			{
				//Draw selected option
				menu.font_bold.draw(&menu.font_bold,
					menu_options[menu.select],
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + (menu.select << 5) - 48 - (menu.scroll >> FIXED_SHIFT),
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(1);
			break;
		}
		case MenuPage_Story:
		{
			static const struct
			{
				const char *week;
				StageId stage;
				const char *name;
				const char *tracks[3];
			} menu_options[] = {
				{NULL, StageId_1_4, "TUTORIAL", {"TUTORIAL", NULL, NULL}},
				{"1", StageId_1_1, "DADDY DEAREST", {"BOPEEBO", "FRESH", "DADBATTLE"}}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = 0;
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.title.fade = FIXED_DEC(0,1);
				menu.page_state.title.fadespd = FIXED_DEC(0,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade >> FIXED_SHIFT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 75, 80);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = true;
					menu.trans_time = FIXED_UNIT;
					menu.page_state.title.fade = FIXED_DEC(255,1);
					menu.page_state.title.fadespd = FIXED_DEC(510,1);
					
					
					//movie stuff
					menu.next_page = MenuPage_Movie;
					movie.select = 0;
					movie.diff = menu.page_param.stage.diff;
					movie.id = menu.page_param.stage.id;
					movie.story = menu.page_param.stage.story;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					menu.next_page = MenuPage_Main;
					menu.next_select = 0; //Story Mode
					Trans_Start();
				}
			}
			
			//Draw week name and tracks
			menu.font_bold.draw(&menu.font_bold,
				menu_options[menu.select].name,
				SCREEN_WIDTH - 16,
				24,
				FontAlign_Right
			);
			
			const char * const *trackp = menu_options[menu.select].tracks;
			for (size_t i = 0; i < COUNT_OF(menu_options[menu.select].tracks); i++, trackp++)
			{
				if (*trackp != NULL)
					menu.font_bold.draw(&menu.font_bold,
						*trackp,
						SCREEN_WIDTH - 16,
						SCREEN_HEIGHT - (4 * 24) + (i * 24),
						FontAlign_Right
					);
			}
			
			//Draw upper strip
			RECT name_bar = {0, 16, SCREEN_WIDTH, 32};
			Gfx_DrawRect(&name_bar, 249, 207, 81);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(48,1);
			menu.scroll += (next_scroll - menu.scroll) >> 3;
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Main)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					s32 y = 64 + (i * 48) - (menu.scroll >> FIXED_SHIFT);
					if (y <= 16)
						continue;
					if (y >= SCREEN_HEIGHT)
						break;
					Menu_DrawWeek(menu_options[i].week, 48, y);
				}
			}
			else if (animf_count & 2)
			{
				//Draw selected option
				Menu_DrawWeek(menu_options[menu.select].week, 48, 64 + (menu.select * 48) - (menu.scroll >> FIXED_SHIFT));
			}
			
			break;
		}
		case MenuPage_Freeplay:
		{
			static const struct
			{
				StageId stage;
				u32 col;
				const char *text;
			} menu_options[] = {
				//{StageId_4_4, 0xFFFC96D7, "TEST"},
				{StageId_1_4, 0xFF9271FD, "TUTORIAL"},
				{StageId_1_1, 0xFF9271FD, "BOPEEBO"},
				{StageId_1_2, 0xFF9271FD, "FRESH"},
				{StageId_1_3, 0xFF9271FD, "DADBATTLE"}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.freeplay.back_r = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_g = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_b = FIXED_DEC(255,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"FREEPLAY",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 100, SCREEN_HEIGHT2 - 48);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					menu.next_page = MenuPage_Main;
					menu.next_select = 1; //Freeplay
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					48 + (y >> 2),
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((menu_options[menu.select].col >> 16) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_g = (fixed_t)((menu_options[menu.select].col >>  8) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_b = (fixed_t)((menu_options[menu.select].col >>  0) & 0xFF) << FIXED_SHIFT;
			
			menu.page_state.freeplay.back_r += (tgt_r - menu.page_state.freeplay.back_r) >> 4;
			menu.page_state.freeplay.back_g += (tgt_g - menu.page_state.freeplay.back_g) >> 4;
			menu.page_state.freeplay.back_b += (tgt_b - menu.page_state.freeplay.back_b) >> 4;
			
			Menu_DrawBack(1);
			break;
		}
		case MenuPage_Mods:
		{
			static const struct
			{
				StageId stage;
				u32 col;
				const char *text;
			} menu_options[] = {
				{StageId_DevilGambit, 0xFF9271FD, "DEVIL'S GAMBIT"},
				{StageId_TooSlow, 0xFF9271FD, "TOO SLOW"}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.freeplay.back_r = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_g = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_b = FIXED_DEC(255,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"CUSTOM SONGS",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 100, SCREEN_HEIGHT2 - 48);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					menu.next_page = MenuPage_Main;
					menu.next_select = 2; //Mods
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					48 + (y >> 2),
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((menu_options[menu.select].col >> 16) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_g = (fixed_t)((menu_options[menu.select].col >>  8) & 0xFF) << FIXED_SHIFT;
			fixed_t tgt_b = (fixed_t)((menu_options[menu.select].col >>  0) & 0xFF) << FIXED_SHIFT;
			
			menu.page_state.freeplay.back_r += (tgt_r - menu.page_state.freeplay.back_r) >> 4;
			menu.page_state.freeplay.back_g += (tgt_g - menu.page_state.freeplay.back_g) >> 4;
			menu.page_state.freeplay.back_b += (tgt_b - menu.page_state.freeplay.back_b) >> 4;
			
			Menu_DrawBack(1);
			break;
		}
		case MenuPage_Options:
		{
			static const char *gamemode_strs[] = {"NORMAL", "SWAP", "TWO PLAYER"};
			static const struct
			{
				enum
				{
					OptType_Boolean,
					OptType_Enum,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
					struct
					{
						s32 max;
						const char **strs;
					} spec_enum;
				} spec;
			} menu_options[] = {
				{OptType_Enum,    "GAMEMODE", &stage.mode, {.spec_enum = {COUNT_OF(gamemode_strs), gamemode_strs}}},
				{OptType_Boolean, "GHOST TAP ", &stage.ghost, {.spec_boolean = {0}}},
				{OptType_Boolean, "DOWNSCROLL", &stage.downscroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "MIDDLESCROLL", &stage.middlescroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "INSTAKILL", &stage.instakill, {.spec_boolean = {0}}},
				{OptType_Boolean, "WIDESCREEN", &stage.widescreen, {.spec_boolean = {0}}}
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
				Audio_LoadMus("\\MUSIC\\OPTIONS.MUS;1");
				Audio_PlayMus(true);
				Audio_SetVolume(0, 0x3FFF, 0x0000);
				Audio_SetVolume(1, 0x0000, 0x3FFF);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"OPTIONS",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				if (pad_state.press & PAD_UP)
				{
					if (menu.select > 0)
						menu.select--;
					else
						menu.select = COUNT_OF(menu_options) - 1;
				}
				if (pad_state.press & PAD_DOWN)
				{
					if (menu.select < COUNT_OF(menu_options) - 1)
						menu.select++;
					else
						menu.select = 0;
				}
				
				//Handle option changing
				switch (menu_options[menu.select].type)
				{
					case OptType_Boolean:
						if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
							*((boolean*)menu_options[menu.select].value) ^= 1;
						break;
					case OptType_Enum:
						if (pad_state.press & PAD_LEFT)
							if (--*((s32*)menu_options[menu.select].value) < 0)
								*((s32*)menu_options[menu.select].value) = menu_options[menu.select].spec.spec_enum.max - 1;
						if (pad_state.press & PAD_RIGHT)
							if (++*((s32*)menu_options[menu.select].value) >= menu_options[menu.select].spec.spec_enum.max)
								*((s32*)menu_options[menu.select].value) = 0;
						break;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					menu.next_page = MenuPage_Main;
					menu.next_select = 3; //Options
					Trans_Start();
					Audio_LoadMus("\\MUSIC\\MENU.MUS;1");
					Audio_PlayMus(true);
					Audio_SetVolume(0, 0x3FFF, 0x0000);
					Audio_SetVolume(1, 0x0000, 0x3FFF);
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) >> 4;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll >> FIXED_SHIFT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				char text[0x80];
				switch (menu_options[i].type)
				{
					case OptType_Boolean:
						sprintf(text, "%s %s", menu_options[i].text, *((boolean*)menu_options[i].value) ? "ON" : "OFF");
						break;
					case OptType_Enum:
						sprintf(text, "%s %s", menu_options[i].text, menu_options[i].spec.spec_enum.strs[*((s32*)menu_options[i].value)]);
						break;
				}
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(text, menu.select != i),
					48 + (y >> 2),
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(1);
			break;
		}
		case MenuPage_Stage:
		{
			//Unload menu state
			Menu_Unload();
			
			//Load new stage
			LoadScr_Start();
			Stage_Load(menu.page_param.stage.id, menu.page_param.stage.diff, menu.page_param.stage.story);
			gameloop = GameLoop_Stage;
			LoadScr_End();
			break;
		}
		case MenuPage_Movie:
		{
			//Unload
			Menu_Unload();

			//Play movie
			LoadScr_Start();
			gameloop = GameLoop_Movie;
			LoadScr_End();
			break;
		}
		default:
			break;
	}
	
	//Clear page swap flag
	menu.page_swap = menu.page != exec_page;
}
