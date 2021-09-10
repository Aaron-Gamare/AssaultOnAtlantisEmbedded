// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/16/2021 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include <stdlib.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
}
//********************************************************************************
#define FIX 32
char score = 0;
int timer = 1000;
void Delay100ms(uint32_t count); // time delay in 0.1 seconds
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum {dead, alive} status_t;
struct sprite{
	int32_t x;
	int32_t y;
	int32_t vx;
	int32_t vy;
	int32_t frame;
	int32_t NumFrame;
	const uint8_t *image[3];
	status_t life;
};

typedef struct sprite sprite_t;

sprite_t Player;
sprite_t tridents[10];
sprite_t bigships[2];
sprite_t mediumships[2];
sprite_t smallships[2];
sprite_t jellyfish[3];
sprite_t fishies[3];
int NeedtoDraw;

void Init(void){
	Player.x = 47*FIX;
	Player.y = 63*FIX;
	Player.image[0] = arrow;
	Player.life = alive;
	
	for(int i =0; i < 10; i++){
		tridents[i].life = dead;
	}
	for(int i =0; i < 2; i++){
		
		bigships[i].frame = 0;
		bigships[i].NumFrame = 3;
		bigships[i].image[0] = bigboi;
		bigships[i].image[1] = ATL_reallybigboi_broken;
		bigships[i].image[2] = ATL_reallybigboi_brokehalf;
		bigships[i].life = dead;
	}
	for(int i =0; i < 2; i++){
		mediumships[i].frame = 0;
		mediumships[i].NumFrame = 2;
		mediumships[i].image[0] = mediumboi;
		mediumships[i].image[1] = ATL_mediumboi_sinking;
		mediumships[i].life = dead;
	}
	for(int i =0; i < 2; i++){
		
		smallships[i].frame = 0;
		smallships[i].NumFrame = 1;
		smallships[i].image[0] = smolboi;
		smallships[i].life = dead;
	}
	for(int i =0; i < 3; i++){
		jellyfish[i].life = dead;
		jellyfish[i].frame = 0;
		jellyfish[i].NumFrame = 1;
	}
	for(int i =0; i < 3; i++){
		fishies[i].life = dead;
		fishies[i].frame = 0;
		fishies[i].NumFrame = 1;
	}
	
}
int countbig = 0;
int countsmall = 0;
int icount = 0;

void Collision(void){
int32_t x1, y1, x2, y2, x3, y3;
	int i = 0;
	for (; i < 10; i++){
		if (tridents[i].life == alive){
			x1 = tridents[i].x + 3*FIX;
			y1 = tridents[i].y -4*FIX;
			icount = i;
			for (int j = 0; j < 2; j++){
				if (bigships[j].life == alive){
					x2 = bigships[j].x + 15*FIX;
					y2 = bigships[j].y - 8*FIX;
					if((abs(x1-x2) < 15*FIX)&&(abs(y1-y2) < 8*FIX)){
						bigships[j].frame++;
						countbig = abs(x1-x2);
						tridents[i].life = dead;
						if(bigships[j].frame == bigships[j].NumFrame){
							bigships[j].life = dead;
							bigships[j].frame = 0;
							score += 5;
							return;
						}
						
					}
				}
			}
			for (int j = 0; j < 2; j++){
				if (mediumships[j].life == alive){
					x2 = mediumships[j].x + 10*FIX;
					y2 = mediumships[j].y - 8*FIX;
					if((abs(x1-x2) < 10*FIX)&&(abs(y1-y2) < 8*FIX)){
						mediumships[j].frame++;
						countsmall++;
						tridents[i].life = dead;
						if(mediumships[j].frame == mediumships[j].NumFrame){
							mediumships[j].life = dead;
							mediumships[j].frame = 0;
							score += 3;
							return;
						}
						
					}
				}
			}
			for (int j = 0; j < 2; j++){
				if (smallships[j].life == alive){
					x2 = smallships[j].x + 5*FIX;
					y2 = smallships[j].y - 6*FIX;
					if((abs(x1-x2) < 5*FIX)&&(abs(y1-y2) < 6*FIX)){
						tridents[i].life = dead;
						smallships[j].life = dead;
						score += 8;
						//count++;
						return;
					}
				}
			}
			for (int j = 0; j < 3; j++){
				if (jellyfish[j].life == alive){
						x2 = jellyfish[j].x + 6*FIX;
						y2 = jellyfish[j].y - 6*FIX;
						if((abs(x1-x2) < 6*FIX)&&(abs(y1-y2) < 6*FIX)){
							tridents[i].life = dead;
							jellyfish[j].life = dead;
							score -= 3;
							return;
						}
						x3 = jellyfish[j+1].x + 6*FIX;
						y3 = jellyfish[j+1].y - 6*FIX;
						if((abs(x2-x3) < 6*FIX)&&(abs(y2-y3) < 6*FIX)){
							jellyfish[j].life = alive;
							jellyfish[j+1].life = dead;
							//count++;
							return;
					}
				}
      }
			for (int j = 0; j < 3; j++){
				if (fishies[j].life == alive){
					x2 = fishies[j].x + 3*FIX;
					y2 = fishies[j].y - 4*FIX;
					if((abs(x1-x2) < 3*FIX)&&(abs(y1-y2) < 4*FIX)){
						tridents[i].life = dead;
						fishies[j].life = dead;
						//count++;
						score -= 2;
						return;
					}
				}
			}
		}
	}
}
////////////////////////////////MOVE//////////////////////////////
void Move(void){
	uint32_t adcData;
	if(Player.life == alive){	
		NeedtoDraw = 1;
		adcData = ADC_In();
		Player.x = ((127-7)*FIX*adcData)/4096; 
	}
	for(int i = 0; i < 10; i++){
		if(tridents[i].life == alive){
			NeedtoDraw = 1;
			if(tridents[i].x < 0){
				tridents[i].x = 2*FIX;
				tridents[i].vx = -tridents[i].vx;
			}
			if(tridents[i].x > 126*FIX){
				tridents[i].x = 124*FIX;
				tridents[i].vx = -tridents[i].vx;
			}
			if((tridents[i].y < 62*FIX) && (tridents[i].y > 0)){
				tridents[i].x += tridents[i].vx;
				tridents[i].y += tridents[i].vy;
			}
			else{
				tridents[i].life = dead;
			}
		}
	}
	
	for(int i = 0; i < 2; i++){
		if(bigships[i].life == alive){
			NeedtoDraw = 1;
			if(bigships[i].x > 126*FIX) {
				bigships[i].life = dead;
				bigships[i].frame = 0;
			}
			if((bigships[i].y < 62*FIX)&& (bigships[i].y > 0)){
				bigships[i].x += bigships[i].vx;
				bigships[i].y += bigships[i].vy;
			}
			else{
				bigships[i].life = dead;
			}
		}
	}
	
	for(int i = 0; i < 2; i++){
		if(mediumships[i].life == alive){
			NeedtoDraw = 1;
			if(mediumships[i].x < 2*FIX) {
				mediumships[i].life = dead;
				mediumships[i].frame = 0;
			}
			if((mediumships[i].y < 62*FIX)&& (mediumships[i].y > 0)){
				mediumships[i].x += mediumships[i].vx;
				mediumships[i].y += mediumships[i].vy;
			}
			else{
				mediumships[i].life = dead;
			}
		}
	}
	for(int i = 0; i < 2; i++){
		if(smallships[i].life == alive){
			NeedtoDraw = 1;
			if(smallships[i].x > 126*FIX) {
				smallships[i].life = dead;
			}
			if((smallships[i].y < 62*FIX)&& (smallships[i].y > 0)){
				smallships[i].x += (smallships[i].vx);
				smallships[i].y += (smallships[i].vy);
			}
			else{
				smallships[i].life = dead;
			}
		}
	}
	for(int i = 0; i < 3; i++){
		if(jellyfish[i].life == alive){
			NeedtoDraw = 1;
			if(jellyfish[i].x < 2*FIX) {
				jellyfish[i].life = dead;
			}
			if((jellyfish[i].y < 62*FIX)&& (jellyfish[i].y > 0)){
				jellyfish[i].x +=  jellyfish[i].vx;
				jellyfish[i].y +=  jellyfish[i].vy;
			}
			else{
				jellyfish[i].life = dead;
			}
		}
	}
		for(int i = 0; i < 3; i++){
		if(fishies[i].life == alive){
			NeedtoDraw = 1;
			if(fishies[i].x > 126*FIX) {
				fishies[i].life = dead;
			}
			if((fishies[i].y < 62*FIX)&& (fishies[i].y > 0)){
				fishies[i].x += fishies[i].vx;
				fishies[i].y += fishies[i].vy;
			}
			else{
				fishies[i].life = dead;
			}
		}
	}
}
/////////////////////////////////FIRE/////////////////////////////
void Fire(int32_t vx1, int32_t vy1){
	int i = 0;
	while(tridents[i].life == alive){
		i++;
		if(i==10){
			return;
		}
	}
	tridents[i].x = Player.x - 1*FIX;
	tridents[i].y = Player.y - 2*FIX;
	tridents[i].vx  = vx1;
	tridents[i].vy  = vy1;
	tridents[i].image[0] = trident;
	tridents[i].life = alive;
	Sound_Shoot();
}

void Spawn(int32_t vx1, int32_t vy1, const uint8_t *im){
	int i = 0;
	//Clock_Delay1ms(20);
	while(bigships[i].life == alive){
		i++;
		if(i == 2) return;
	}
	bigships[i].x = 2*FIX;
	bigships[i].y = 7*FIX;
	bigships[i].vx = vx1;
	bigships[i].vy = vy1;
	bigships[i].image[0] = im;
	bigships[i].life = alive;
}

void Spawn1(int32_t vx1, int32_t vy1, const uint8_t *im){
	int j = 0;
	//Clock_Delay1ms(20);
	while(mediumships[j].life == alive){
		j++;
		if(j == 2) return;
	}
	mediumships[j].x = 106*FIX;
	mediumships[j].y = 18*FIX;
	mediumships[j].vx = vx1;
	mediumships[j].vy = vy1;
	mediumships[j].image[0] = im;
	mediumships[j].life = alive;
}

void Spawn2(int32_t vx1, int32_t vy1, const uint8_t *im){
	int j = 0;
	//Clock_Delay1ms(20);
	while(smallships[j].life == alive){
		j++;
		if(j == 2) return;
	}
	smallships[j].x = 2*FIX;
	smallships[j].y = 28*FIX;
	smallships[j].vx = vx1;
	smallships[j].vy = vy1;
	smallships[j].image[0] = im;
	smallships[j].life = alive;
}

void Spawn3(int32_t vx1, int32_t vy1, const uint8_t *im){
	int j = 0;
	//Clock_Delay1ms(20);
	while(jellyfish[j].life == alive){
		j++;
		if(j == 3) return;
	}
	jellyfish[j].x = 106*FIX;
	jellyfish[j].y = 38*FIX;
	jellyfish[j].vx = vx1;
	jellyfish[j].vy = vy1;
	jellyfish[j].image[0]= im;
	jellyfish[j].life = alive;
}

void Spawn4(int32_t vx1, int32_t vy1, const uint8_t *im){
	int j = 0;
	//Clock_Delay1ms(20);
	while(fishies[j].life == alive){
		j++;
		if(j == 3) return;
	}
	fishies[j].x = 2*FIX;
	fishies[j].y = 46*FIX;
	fishies[j].vx = vx1;
	fishies[j].vy = vy1;
	fishies[j].image[0] = im;
	fishies[j].life = alive;
}
////////////////////////////SYSTICK_HANDLER///////////////////////
int lang = 0;
void SysTick_Handler(void){
	static uint32_t lastdown = 0;
	uint32_t down = Switch_In() & 0x04;
	
	if((Switch_In()&0x08)==0x08){ 
		SSD1306_OutClear();
		SSD1306_OutBuffer();
		SSD1306_SetCursor(7,6);
		if(lang == 0){
				SSD1306_OutClear();
				SSD1306_OutBuffer();
				SSD1306_SetCursor(7,6);
				SSD1306_OutString("Paused");
		}
		if(lang == 1){
			SSD1306_OutClear();
			SSD1306_OutBuffer();
			SSD1306_SetCursor(7,6);
			SSD1306_OutString("In Pausa");
		}
		Delay100ms(5);
		while ((Switch_In()&0x08)==0x0){};
		Delay100ms(5);
		while ((Switch_In()&0x08)==0x08){};
  }
	if((down == 0x04)&& (lastdown == 0)){
		Fire(0,-2*FIX);
	}
	if(Random() < 5){
		Spawn(FIX, 0, bigboi);
	}
	if(Random() < 3){	
		Spawn1(-1.25 * FIX,0, mediumboi);
	}
	if(Random() < 2){
		Spawn2(4 * FIX, 0, smolboi);
	}
	if((Random()) < 3){
		Spawn3(-0.5 * FIX, 0, ATL_jelley);
	}
	if((Random()) < 3){	
		Spawn4(0.5 * FIX, 0, fish);
	}
	Move();
	lastdown = down;
	timer--;
}
/////////////////////////////////DRAW/////////////////////////////
uint32_t m = 0;
void Draw(void){
	SSD1306_ClearBuffer();
	m = Random()%60;
	if(Player.life == alive){
		SSD1306_DrawBMP(Player.x/FIX, Player.y/FIX, Player.image[0], 0, SSD1306_INVERSE);
	}
	for(int i = 0; i < 10; i++){
		if(tridents[i].life){
			SSD1306_DrawBMP(tridents[i].x/FIX, tridents[i].y/FIX, tridents[i].image[0], 0, SSD1306_INVERSE);
		}
	}
	for(int i = 0; i < 2; i++){
		if(bigships[i].life){
			SSD1306_DrawBMP(bigships[i].x/FIX, bigships[i].y/FIX, bigships[i].image[bigships[i].frame], 0, SSD1306_INVERSE);
		}
	}
	for(int i = 0; i < 2; i++){
		if(mediumships[i].life){
			SSD1306_DrawBMP(mediumships[i].x/FIX, mediumships[i].y/FIX, mediumships[i].image[mediumships[i].frame], 0, SSD1306_INVERSE);
		}
	}
	for(int i = 0; i < 2; i++){
		if(smallships[i].life){
			SSD1306_DrawBMP(smallships[i].x/FIX, smallships[i].y/FIX, smallships[i].image[0], 0, SSD1306_INVERSE);
		}
	}
	for(int i = 0; i < 2; i++){
		if(jellyfish[i].life){
			SSD1306_DrawBMP(jellyfish[i].x/FIX, jellyfish[i].y/FIX, jellyfish[i].image[0], 0, SSD1306_INVERSE);
		}
	}
	for(int i = 0; i < 3; i++){
		if(fishies[i].life){
			SSD1306_DrawBMP(fishies[i].x/FIX, fishies[i].y/FIX, fishies[i].image[0], 0, SSD1306_INVERSE);
		}
	}
	SSD1306_OutBuffer();
	NeedtoDraw = 0;		//semaphore
}


void SysTick_Init20Hz(void);
//////////////////////////////////////main//////////////////////////////////////////

int test = 0;
int test2 = 0;
int main(void){
	DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
  TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();   
  ADC_Init();
	Random_Init(ADC_In());
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
	SSD1306_DrawBMP(2, 63, titlepage, 0, SSD1306_WHITE);
	SSD1306_OutBuffer();
  Delay100ms(30);
	Switch_Init();
	SSD1306_OutClear();
  SSD1306_OutBuffer();
  SSD1306_SetCursor(4,2);
  SSD1306_OutString("Language Select");
  SSD1306_SetCursor(5,4);
  SSD1306_OutString("L - English");
  SSD1306_SetCursor(5,5);
  SSD1306_OutString("R - Italiano");
	while((Switch_In()&0x04) == 0 && (Switch_In()&0x08) == 0){};
	
	if((Switch_In()&0x04)==0x04){
		SSD1306_OutClear();
		SSD1306_OutBuffer();
		lang = 0;
		SSD1306_SetCursor(5,0);
		SSD1306_OutString("Instructions:");
		SSD1306_SetCursor(0,2);
		SSD1306_OutString("Slide to Move");
		SSD1306_SetCursor(0,3);
		SSD1306_OutString("L to Fire ");
		SSD1306_SetCursor(0,4);
		SSD1306_OutString("R to Pause ");
		SSD1306_SetCursor(0,5);
		SSD1306_OutString("You have 50 Seconds ");
		SSD1306_SetCursor(0,6);
		SSD1306_OutString("Press R to Begin ");
		while((Switch_In()&0x08) == 0){};
		Delay100ms(20);
	}
	if((Switch_In()&0x08)==0x08){
		SSD1306_OutClear();
		SSD1306_OutBuffer();
		lang = 1;
		SSD1306_SetCursor(5,0);
		SSD1306_OutString("Istruzioni:");
		SSD1306_SetCursor(0,2);
		SSD1306_OutString("Scorri per Muoverti");
		SSD1306_SetCursor(0,3);
		SSD1306_OutString("L sparare");
		SSD1306_SetCursor(0,4);
		SSD1306_OutString("R pausa");
		SSD1306_SetCursor(0,5);
		SSD1306_OutString("Hai 50 secondi");
		SSD1306_SetCursor(0,6);
		SSD1306_OutString("Premer R per iniziare");
		Delay100ms(10);	
		while((Switch_In()&0x08) == 0){};
		Delay100ms(10);
	}
  SysTick_Init20Hz();
	Sound_Init();
	Init();
	
	EnableInterrupts();
	while(timer > 0){
		if(NeedtoDraw){
			Draw();
		}
		Collision();
		SSD1306_SetCursor(19,7);
		SSD1306_OutUDec2(score);
		SSD1306_SetCursor(0,7);
		SSD1306_OutUDec2(timer/20);
	}
	SSD1306_ClearBuffer();
	SSD1306_OutBuffer();
	if(lang == 0){
		SSD1306_SetCursor(6,3);
		SSD1306_OutString("GAME OVER");
		SSD1306_SetCursor(6,4);
		SSD1306_OutString("Score: ");
		SSD1306_OutUDec2(score);
	}
	if(lang == 1){
		SSD1306_SetCursor(6,3);
		SSD1306_OutString("GIOCO FINITO");
		SSD1306_SetCursor(6,4);
		SSD1306_OutString("Punto: ");
		SSD1306_OutUDec2(score);
	}
}

void SysTick_Init20Hz(void){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = (80000000/20)-1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_ST_CTRL_R = 7;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF)|0x40000000;
}



// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
