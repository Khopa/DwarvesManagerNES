/**
 * 
 * LUDUM DARE 29 - DWARVES MANAGER 2
 * by Clement "Khopa" Perreau  
 *
 * using neslib by Shiru (Thank you guy, you're awesome !)
 *
 * <troll>BECAUSE UNITY IS FOR NOOBS</troll>
 * Oh ! There is actually someone reading this source code ?
 * Thanks, but it's really messy down there ! :)
 *
 */

#include "neslib.h"
#include "title.h"
#include "map.h"
#include "instructions.h"
#include "sprites.h"

#define NTADR(x,y) ((0x2000|((y)<<5)|x))
#define MSB(x)		(((x)>>8))
#define LSB(x)		(((x)&0xff))

#define PLAYER_A 0x00
#define ST_MENU  0x00
#define ST_GAME  0x01
#define ST_INSTRUCTIONS  0x02

#define DWARVES_COUNT 0x03
#define DWARVES_SPRITE_SIZE 37

#define SCREEN_X_SIZE 256
#define SCREEN_Y_SIZE 240

#define TOP 5*8
#define BOT 22*8
#define LEF 6*8
#define RIG 23*8

// There is 9 sprites per dwarves
// 64 sprites is the max the NES can handle
#define FIREBALL_MAX 64-DWARVES_COUNT*9

// static
static unsigned char pad;
static unsigned int STATE;

// GOLD 
static unsigned int GOLD = 0;

// DWARVES MANAGEMENT
static unsigned char   X_POS[DWARVES_COUNT];
static unsigned char   Y_POS[DWARVES_COUNT];
static unsigned char   ALIVE[DWARVES_COUNT];
static unsigned int  selected;

// FIREBALLS
static int FIREBALL_X[FIREBALL_MAX];
static int FIREBALL_Y[FIREBALL_MAX];
static char FIREBALL_DX[FIREBALL_MAX];
static char FIREBALL_DY[FIREBALL_MAX];
static unsigned char FIREBALL_LIVE[FIREBALL_MAX];
static unsigned char FIREBALL_ALIVE[FIREBALL_MAX];

// Update list
static unsigned char ulist[4*3];
const unsigned char updateListData[4*3]={
	MSB(NTADR(12,2)),LSB(NTADR(12,2)),0,
	MSB(NTADR(13,2)),LSB(NTADR(13,2)),0,
	MSB(NTADR(14,2)),LSB(NTADR(14,2)),0,
	MSB(NTADR(15,2)),LSB(NTADR(15,2)),0,
};

// Sprite palette
const unsigned char spritePalette[16]={ 0x0f,0x06,0x16,0x27,0x0f,0x17,0x16,0x07,0x0f,0x27,0x26,0x16,0x0f,0x0f,0x21,0x30 };

/**
 * PUT A STRING ON SCREEN
 */
void put_str(unsigned int adr,const char *str){
	vram_adr(adr);
	while(1)
	{
		if(!*str) break;
		vram_put((*str++)-0x20); //-0x20 because ASCII code 0x20 is placed in tile 0 of the CHR
	}
}

void setMenuState(){
	oam_clear();
	ppu_off();
	STATE = ST_MENU;
	set_vram_update(NULL);
	vram_unrle(title);
	pal_bright(4);
	put_str(NTADR(6,16), "PRESS START TO BEGIN");
	ppu_on_all();
}


void setInstructionsState(){
	oam_clear();
	ppu_off();
	STATE = ST_INSTRUCTIONS;
	set_vram_update(NULL);
	vram_unrle(instructions);
	pal_bright(4);
	ppu_on_all();
}

void setGameState(){
	unsigned char i = 0;
	oam_clear();
	ppu_off();
	STATE = ST_GAME;
	vram_unrle(map);

	// init dwarves
	for(i=0;i<DWARVES_COUNT;i++){
		X_POS[i] = 80 + i*35;
		Y_POS[i] = 100;
		ALIVE[i] = 1;
	}

	// init fireballs
	for(i = 0;i < FIREBALL_MAX; i++){
		FIREBALL_ALIVE[i] = 0;
	}
	GOLD = 0;
	selected = 0;
	pal_spr(spritePalette);

	memcpy(ulist,updateListData,sizeof(updateListData));
	set_vram_update(ulist);

	ppu_on_all();
}

void spawnFireball(){
	unsigned char i = 0;
	unsigned char rndX = 0;
	unsigned char rndY = 0;
	unsigned char rndSide = rand8()%4;
	unsigned char speed = 1;
	if(GOLD > 250){
		speed = rand8()%3+1;
	}else if(GOLD > 125){
		speed = rand8()%2+1;
	}
	for(i = 0;i < FIREBALL_MAX; i++){
		if(!FIREBALL_ALIVE[i]){
			switch(rndSide){
				case 0: // SIDE UP
					rndY = 0;
					rndX = rand8()%(RIG-LEF)+LEF;
					if(GOLD < 100){
						FIREBALL_DX[i] = rand8()%(5)-2;
					}else{
						FIREBALL_DX[i] = 0;
					}
					FIREBALL_DY[i] = speed;
				break;
				case 1: // SIDE DOWN
					rndY = SCREEN_Y_SIZE;
					rndX = rand8()%(RIG-LEF)+LEF;
					if(GOLD < 100){
						FIREBALL_DX[i] = rand8()%(5)-2;
					}else{
						FIREBALL_DX[i] = 0;
					}
					FIREBALL_DY[i] = -speed;
				break;
				case 2: // SIDE LEFT
					rndY = rand8()%(BOT-TOP)+TOP;
					rndX = 0;
					FIREBALL_DX[i] = speed;
					if(GOLD < 100){
						FIREBALL_DY[i] = rand8()%(5)-2;
					}else{
						FIREBALL_DY[i] = 0;
					}
				break;
				default: // SIDE RIGHT
					rndY = rand8()%(BOT-TOP)+TOP;
					rndX = SCREEN_X_SIZE;
					FIREBALL_DX[i] = -speed;
					if(GOLD < 100){
						FIREBALL_DY[i] = rand8()%(5)-2;
					}else{
						FIREBALL_DX[i] = 0;
					}
				break;
			}
			FIREBALL_X[i] = rndX;
			FIREBALL_Y[i] = rndY;
			FIREBALL_LIVE[i] = 0;
			FIREBALL_ALIVE[i] = 1;
			break;
		}
	}
}

/**
 * Fireball count
 */
char fireballCount(){
	char count = 0;
	char i = 0;
	for(i = 0;i < FIREBALL_MAX; i++){
		if(FIREBALL_ALIVE[i]) count++;
	}
	return count;
}

/**
 * Dwarves count
 */
char dwarvesCount(){
	char count = 0;
	char i = 0;
	for(i = 0;i < DWARVES_COUNT; i++){
		if(ALIVE[i]) count++;
	}
	return count;
}

/**
 * Select the next dwarf
 */
void selectNextDwarf(){
	char count = 0;
	char i = 0;;
	selected = selected + 1;
	if(selected>=DWARVES_COUNT) selected = 0;
	if(ALIVE[selected] == 1) return;
	while(!(ALIVE[selected]) == 1){
		selected = selected + 1;
		if(selected>=DWARVES_COUNT) selected = 0;
		if(ALIVE[selected] == 1) return;
		count++;
		if(count>DWARVES_COUNT) return;
	}
	return;
}


/**
 * MAIN
 */
void main(void)
{
	unsigned int frame = 0;
	unsigned int delay = 0;
	unsigned int frameWithoutMove = 0;
	unsigned char animFrame = 0;
	unsigned char animCounter = 0;
	unsigned char canMine = 0;
	unsigned char aDown = 0;
	unsigned char endTimer = 0;
	unsigned char selectDown = 0;
	int tmp = 0;
	unsigned char i = 0;
	unsigned char j = 0;
	unsigned char xTitle = 80;
	unsigned char goingRight = 1;
	unsigned char specialEffect = 0;
	unsigned char xTmp = 0;
	unsigned char yTmp = 0;
	unsigned char maxSpr = 0;
	unsigned char bright = 4;
	unsigned char brightnessInc = 1;
	int spr = 0;

	// Set palette colors
	for(i = 0; i<8; i++){
		tmp = i*4;
		pal_col(tmp,0x0F);
		pal_col(tmp+1,0x06);
		pal_col(tmp+2,0x16);
		pal_col(tmp+3,0x27);	
	}
	
	setMenuState();

	ppu_on_all();


	while(1){
		delay = delay + 1;
		
		ppu_wait_nmi();

		switch(STATE){
			case ST_MENU:
				GOLD = 0;
				bright = 4;
				if(goingRight){
					xTitle = xTitle+1;
					if(xTitle>170) goingRight = 0;
				}else{
					xTitle = xTitle-1;
					if(xTitle<60) goingRight = 1;
				}
				oam_meta_spr(xTitle,80,0,DWARVE_SPRITE_DATA[0]);
				pad=pad_trigger(PLAYER_A); // PAD for player 1
				if(pad&PAD_START){
					bright = 4;
					endTimer = 0;
					setInstructionsState();
				}
			break;
			case ST_GAME:
				frame++;
				animFrame++;

				// Poll pad
				pad=pad_poll(PLAYER_A);

				if(frame > 24 && bright < 4 && endTimer == 0)
				{
					bright++;
					pal_bright(bright);
					frame = 0;
				}
				else if(endTimer > 0 && frame > 66 && bright > 0){
					bright--;
					pal_bright(bright);
					frame = 0;
				}

				// Dwarves selection
				if(pad&PAD_SELECT){
					if(selectDown == 0){
						selectNextDwarf();
						selectDown = 1;
					}
				}else{
					selectDown = 0;
				}

				// Controlling dwarf
				tmp = 1;
				if(pad&PAD_B) tmp = 2;
				if(pad&PAD_LEFT)  X_POS[selected] = X_POS[selected]-tmp;
				if(pad&PAD_RIGHT) X_POS[selected] = X_POS[selected]+tmp;
				if(pad&PAD_UP)    Y_POS[selected] = Y_POS[selected]-tmp;
				if(pad&PAD_DOWN)  Y_POS[selected] = Y_POS[selected]+tmp;


				// ULTRA BASIC COLLISION FOR WALLS
				if(X_POS[selected] < LEF){
					X_POS[selected] = LEF;
				}else if(X_POS[selected] > RIG){
					X_POS[selected] = RIG;
				}

				if(Y_POS[selected] < TOP){
					Y_POS[selected] = TOP;
				}else if(Y_POS[selected] > BOT){
					Y_POS[selected] = BOT;
				}

				// Fireballs spawning
				tmp = fireballCount();
				if(GOLD < 10){
					// do nothing
				}else if(GOLD < 35){
					if(rand8()>250 & tmp<5){
						spawnFireball();
					}
				}else if(GOLD < 100){
					if(rand8()>245 & tmp<7){
						spawnFireball();
					}
				}else if(GOLD < 150){
					if(rand8()>240 & tmp<12){
						spawnFireball();
					}
				}else if(GOLD < 225){
					if(rand8()>235 & tmp<15){
						spawnFireball();
					}
				}else{
					if(rand8()>220 & tmp<25){
						spawnFireball();
					}
				}

				// Display dwarves sprite
				spr = 0;
				for(i=0;i<DWARVES_COUNT;i++){
					if(ALIVE[i] == 1){
						if(selected == i){
							if(animFrame >= 10){
								animCounter ^=1;
								animFrame = 0;
							}
							if(animCounter){
								spr = oam_meta_spr(X_POS[i],Y_POS[i],spr,DWARVE_SPRITE_SELECTED_DATA_ANIM[i]);	
							}else{
								spr = oam_meta_spr(X_POS[i],Y_POS[i],spr,DWARVE_SPRITE_SELECTED_DATA[i]);	
							}
						}
						else{
							spr = oam_meta_spr(X_POS[i],Y_POS[i],spr,DWARVE_SPRITE_DATA[i]);		
						}
					}
				}


				// Display fireballs
				for(i = 0; i < FIREBALL_MAX; i++){
					if(FIREBALL_ALIVE[i]){
						spr = oam_spr(FIREBALL_X[i],FIREBALL_Y[i],0x40,4,spr);
						FIREBALL_X[i] = FIREBALL_X[i] + FIREBALL_DX[i];
						FIREBALL_Y[i] = FIREBALL_Y[i] + FIREBALL_DY[i];
						FIREBALL_LIVE[i] = FIREBALL_LIVE[i] + 1;
						if(FIREBALL_LIVE[i] > 128){
							FIREBALL_ALIVE[i] = 0;
						}
						// DO collision with dwarves in the same loop
						for(j=0;j<DWARVES_COUNT;j++){
							if(ALIVE[j] && FIREBALL_X[i]+4 > X_POS[j]+4 && FIREBALL_X[i]+4 < X_POS[j]+20){
								if(FIREBALL_Y[i]+4 > Y_POS[j]+4 && FIREBALL_Y[i]+4 < Y_POS[j]+20){
									ALIVE[j] = 0;
									FIREBALL_ALIVE[j] = 0;
									if(dwarvesCount() > 0) bright = 1;
									else bright = 5;
									selectNextDwarf();
								}
							}
						}
					}
				}

				if(spr > maxSpr){
					maxSpr = spr;
				}else{
					// Clear sprites
					while(spr<maxSpr){
						spr = oam_spr(0,0,0,2,spr);
					}
				}

				// GOLD MINING SYSTEM
				if(pad&(PAD_LEFT|PAD_DOWN|PAD_UP|PAD_RIGHT|PAD_SELECT)){
					frameWithoutMove = 0;
					canMine = 0;
				}
				else{
					frameWithoutMove ++;
					if((frameWithoutMove > 1 || canMine) && (pad&PAD_A && !aDown) && dwarvesCount() > 0){
						GOLD+=1;
						frameWithoutMove = 0;
						canMine = 1;
						// Move dwarves randomly when mining is successful
						xTmp = rand8()%2;
						if(frame&1){
							tmp = 1;
						}else{
							tmp = -1;
						}
						switch(xTmp){
							case 0:
								X_POS[selected]+=tmp;
							break;
							case 1:
								Y_POS[selected]+=tmp;
							break;
						}
						
					}
				}
				if(pad&PAD_A){
					aDown=1;
				}else{
					aDown=0;
				}


				// Display GOLD score
				// TODO : resolve counter glitch here
				if(GOLD > 999) GOLD = 999;
				ulist[2] =0x10;
				ulist[5] =0x10+GOLD/100;
				ulist[8] =0x10+GOLD/10%10;
				ulist[11]=0x10+GOLD%10;

				// Check dwarves
				if(dwarvesCount()<=0){
					endTimer++;
					if(endTimer > 125){
						setMenuState();
					}
				}


			break;
			case ST_INSTRUCTIONS:
				pad=pad_trigger(PLAYER_A); // PAD for player 1
				if(pad&PAD_START){
					setGameState();
				}
			break;
		}


	}
}
