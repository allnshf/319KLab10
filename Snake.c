// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 11/20/2018 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2018

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2018

 Copyright 2018 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

typedef enum {left,right,up,down} dir_t;		//Direction that part of snake will travel next
struct gridSpot{
	uint8_t row;						//Grid array index
	uint8_t col;
	uint32_t x;							//x pixel coordinate
	uint32_t y;							//y pixel coordinate
	const uint16_t *image;	//What to draw in spot
	uint8_t fill;						//1 if grid is occupied, 0 if is empty
	dir_t dir;
};
typedef struct gridSpot tile_t;


tile_t grid[18][16];
tile_t head;
tile_t tail;
int score;

/**	Initialize button inputs	
*/
void PortE_Init(){
	volatile int delay;
	SYSCTL_RCGCGPIO_R |= 0x10;
	delay = 420;
	GPIO_PORTE_DIR_R &= ~0xF;
	GPIO_PORTE_DEN_R |= 0xF;
}

/**	Fills a random, unoccupied spot with an apple
*/
void fillFood(void){
	uint8_t row,col;
	do{
	 row = Random32() % 18;
	 col = Random32() % 16;
	}while(grid[row][col].fill == 1);
	grid[row][col].image = Apple;
	grid[row][col].fill = 1;
	ST7735_DrawBitmap(grid[row][col].x, grid[row][col].y, grid[row][col].image, 8,8);
}

/**	Fills a random, unoccupied spot with a rotton food
*/
void fillRotten(void){
	uint8_t row,col;
	do{
	 row = Random32() % 18;
	 col = Random32() % 16;
	}while(grid[row][col].fill == 1);
	grid[row][col].image = RottenFood;
	grid[row][col].fill = 1;
	ST7735_DrawBitmap(grid[row][col].x, grid[row][col].y, grid[row][col].image, 8,8);
}

/**	Game ends, score is displayed
*/
void gameOver(){
	for(int i = 0; i < 24000000;i++){} //INSERT DEATH SONG
	ST7735_FillScreen(0x0000);    
	ST7735_SetCursor(0,0);
	ST7735_OutString("GAME OVER");
	ST7735_SetCursor(0,1);
	ST7735_OutString("Score: ");
	LCD_OutDec(score);
	ST7735_SetTextColor(31);
	while(1);							//WORK IN PROGRESS
}

/**	Initialize grid for game
*/
void grid_Init(){
	for(int row = 0; row < 18; row++){
		for(int col = 0; col < 16; col++){
			grid[row][col].row = row;
			grid[row][col].col = col;
			grid[row][col].x = col * 8;
			grid[row][col].y = (row + 2) * 8;
			grid[row][col].fill = 0;
		}
	}
	grid[10][1].image = SnakeHorizontal;
	grid[10][1].fill = 1;
	grid[10][1].dir = right;
	grid[10][2].image = SnakeHorizontal;
	grid[10][2].fill = 1;
	grid[10][2].dir = right;
	grid[10][3].image = SnakeHorizontal;
	grid[10][3].fill = 1;
	grid[10][3].dir = right;
	fillFood();
	fillRotten();
	fillRotten();
	head = grid[10][3];
	tail = grid[10][1];
	for(int row = 0; row < 18; row++){
		for(int col = 0; col < 16; col++){
			if(grid[row][col].fill == 1){
				ST7735_DrawBitmap(grid[row][col].x, grid[row][col].y, grid[row][col].image, 8,8);
			}		
		}
	}
	ST7735_SetCursor(0,0);
	ST7735_OutString("Score: ");
	LCD_OutDec(score);
	ST7735_DrawFastHLine(0, 8, 128, 32767);
}

/**	Initialize SysTick to move Snake
*/
void SysTick_Init(int32_t speed){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = (-2447 * speed) + 24300000;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
	NVIC_ST_CTRL_R |= 0x07;
}

void changeSpeed(){
	uint32_t Data = ADC_In();
	SysTick_Init(Data);
}

/**	Checks the direction to move based on input
		Updates head's coordinates, image, and current traveling direction
		Returns: new head
*/
tile_t checkDirection(tile_t current){
	dir_t direction = current.dir;
	if(GPIO_PORTE_DATA_R == 1){		//PE0 => down
		if(direction == down)
			current.image = SnakeVertical;
		else if(direction == up)
			gameOver();
		else if(direction == left)
			current.image = SnakeCornerUR;
		else if(direction == right)
			current.image = SnakeCornerUL;
		grid[current.row][current.col].dir = down;
		ST7735_DrawBitmap(current.x, current.y, current.image, 8,8);	//Update head
		current.row++;							//Update coordinates
		current.y += 8;
		current.image = SnakeVertical;
		current.dir = down;
	}
	if(GPIO_PORTE_DATA_R == 2){		//PE1 => up
		if(direction == up)
			current.image = SnakeVertical;
		else if(direction == down)
			gameOver();
		else if(direction == left)
			current.image = SnakeCornerDR;
		else if(direction == right)
			current.image = SnakeCornerDL;
		grid[current.row][current.col].dir = up;
		ST7735_DrawBitmap(current.x, current.y, current.image, 8,8);	//Update head
		current.row--;							//Update coordinates
		current.y -= 8;
		current.image = SnakeVertical;
		current.dir = up;
	}
	if(GPIO_PORTE_DATA_R == 4){		//PE2 => left
		if(direction == down)
			current.image = SnakeCornerDL;
		else if(direction == up)
			current.image = SnakeCornerUL;
		else if(direction == left)
			current.image = SnakeHorizontal;
		else if(direction == right)
			gameOver();
		grid[current.row][current.col].dir = left;
		ST7735_DrawBitmap(current.x, current.y, current.image, 8,8);	//Update head
		current.col--;							//Update coordinates
		current.x -= 8;
		current.image = SnakeHorizontal;
		current.dir = left;
	}
	if(GPIO_PORTE_DATA_R == 8){		//PE3 => right
		if(direction == down)
			current.image = SnakeCornerDR;
		else if(direction == up)
			current.image = SnakeCornerUR;
		else if(direction == left)
			gameOver();
		else if(direction == right)
			current.image = SnakeHorizontal;
		grid[current.row][current.col].dir = right;
		ST7735_DrawBitmap(current.x, current.y, current.image, 8,8);	//Update head
		current.col++;							//Update coordinates
		current.x += 8;
		current.image = SnakeHorizontal;
		current.dir = right;
	}
	if(GPIO_PORTE_DATA_R == 0){
		if(direction == down){
			current.image = SnakeVertical;
			current.row++;
			current.y += 8;
		}
		else if(direction == up){
			current.image = SnakeVertical;
			current.row--;
			current.y -= 8;
		}
		else if(direction == left){
			current.image = SnakeHorizontal;
			current.col--;							
			current.x -= 8;
		}
		else if(direction == right){
			current.image = SnakeHorizontal;
			current.col++;							
			current.x += 8;
		}
	}
	current.fill = 1;
	return current;
}

/**	Resolves a collision in the game (snake,food,poison)
		Return- collision result (1 for food, 0 for poison)
*/
int collision(tile_t current){
		if(grid[current.row][current.col].image == Apple){
			score++;
			ST7735_SetCursor(0,0);
			ST7735_OutString("Score: ");
			LCD_OutDec(score);
			return 1;
		}
		if(grid[current.row][current.col].image == RottenFood){
			tail.image = Blank;
			tail.fill = 0;
			ST7735_DrawBitmap(tail.x, tail.y, tail.image, 8,8);		//Erase tail
			grid[tail.row][tail.col] = tail;								//Update tail in grid
			if(tail.dir == right)															//Update new tail
				tail = grid[tail.row][tail.col + 1];
			else if(tail.dir == left)
				tail = grid[tail.row][tail.col - 1];
			else if(tail.dir == up)
				tail = grid[tail.row - 1][tail.col];
			else if(tail.dir == down)
				tail = grid[tail.row + 1][tail.col];
			if(score == 0)
				gameOver();
			score--;
			ST7735_SetCursor(0,0);
			ST7735_OutString("Score: ");
			LCD_OutDec(score);
			return 0;
		}
		else
			gameOver();
		return -1;
}

/**	Moves the snake one pixel, detecting if colllision would occur
*/
void move(){		
	tile_t temp = checkDirection(head);					//checkDirection direction input moves snake		
	int collided = -1;
	if(grid[temp.row][temp.col].fill == 1)	//Look if collision occurs
		collided = collision(temp);
	if(temp.row > 15 || temp.col > 17)			//Look if out of boundary occurs
		gameOver();
	ST7735_DrawBitmap(temp.x, temp.y, temp.image, 8,8);	//Draw new head
	grid[temp.row][temp.col] = temp;										//Update value in grid
	head = temp;														//Update head
	if(collided == 1)
		fillFood();
	if(collided == 0){
		fillRotten();
		fillRotten();
	}
	
	if(collided != 1){
		tail.image = Blank;
		tail.fill = 0;
		ST7735_DrawBitmap(tail.x, tail.y, tail.image, 8,8);		//Erase tail
		grid[tail.row][tail.col] = tail;								//Update tail in grid
		if(tail.dir == right)															//Update new tail
			tail = grid[tail.row][tail.col + 1];
		else if(tail.dir == left)
			tail = grid[tail.row][tail.col - 1];
		else if(tail.dir == up)
			tail = grid[tail.row - 1][tail.col];
		else if(tail.dir == down)
			tail = grid[tail.row + 1][tail.col];
	}
}

void SysTick_Handler(){
	move();
	changeSpeed();
}

int main(void){
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
  Random_Init(1);
	PortE_Init();
	ADC_Init();
  Output_Init();
  ST7735_FillScreen(0x0000);            // set screen to black
  
  grid_Init();
	SysTick_Init(ADC_Init());
	score = 0;

  while(1){
		
  }
	
}




