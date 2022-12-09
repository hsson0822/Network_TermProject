#include "Fish.h"
#include "../../Network_TermProject_Server/Network_TermProject_Server/protocol.h"

#include <iostream>
#include <bitset>

Fish::Fish()
{
	width = FISH_INIT_WIDTH;
	height = FISH_INIT_HEIGHT;
	age = 0;
	exp = 0;
	maxExp = 20;
	rect = { 0,0,0 + width, 0 + height };
	moveDir = 0;
	x = 0;
	y = 0;
	is_active = false;
	is_caught = false;
	speed = FISH_INIT_SPEED;
}

Fish::Fish(int posX, int posY) : Fish()
{
	Fish();
	rect = { posX,posY,posX + width, posY + height };
}


RECT Fish::getRect() { return rect; }
void Fish::setRect(RECT r) { rect = r; }



int Fish::getAge() { return age; }
void Fish::addAge()
{
	width += 30;
	height += 30;
	rect.right += 30;
	rect.bottom += 30;
}


int Fish::getExp() { return exp; }
void Fish::setExp(int e) { exp = e; }
int Fish::getMaxExp() { return maxExp; }

int Fish::getWidth() { return width; }
void Fish::setWidth(int w) { width = w; }

int Fish::getHeight() { return height; }
void Fish::setHeight(int h) { height = h; }

int Fish::getMoveCount() { return moveCount; }
void Fish::addMoveCount() { ++moveCount; }
void Fish::resetMoveCount() { moveCount = 0; }

unsigned char Fish::getMoveDir() { return moveDir;}
void Fish::setMoveDir(unsigned char dir) 
{ 
	if ((dir & MOVE_LEFT) == MOVE_LEFT) {
		setXY(true);
		setLR(false);
	}
	if ((dir & MOVE_RIGHT) == MOVE_RIGHT) {
		setXY(true);
		setLR(true);
	}
	if ((dir & MOVE_UP) == MOVE_UP) {
		setXY(false);
		setUD(false);
	}
	if ((dir & MOVE_DOWN) == MOVE_DOWN) {
		setXY(false);
		setUD(true);
	}
	
	moveDir = dir; 
}

int Fish::GetSpeed() const { return speed; }
void Fish::SetSpeed(int value) { speed = value; }

BOOL Fish::isXY() { return goXY; }
BOOL Fish::isLR() { return goLR; }
BOOL Fish::isUD() { return goUD; }

void Fish::setXY(bool xy) { goXY = xy; }
void Fish::setLR(bool lr) { goLR = lr; }
void Fish::setUD(bool ud) { goUD = ud; }

void Fish::Move(short posX, short posY)
{
	/*if (posX > x)
		goLR = true;
	else if(posX < x)
		goLR = false;*/

	x = posX;
	y = posY;
	setRect(RECT{ x, y, x + width, y + height });
}

void Fish::Draw(const HDC& memDC1, const HDC& memDC2, HBITMAP image, HBITMAP arrow)
{
	if (is_active) {
		HBITMAP Bit = (HBITMAP)SelectObject(memDC2, image);
		if (isLR())
			TransparentBlt(memDC1, getRect().left, getRect().top, getWidth(), getHeight(), memDC2, 124 * getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
		else
			TransparentBlt(memDC1, getRect().left, getRect().top, getWidth(), getHeight(), memDC2, 124 * getMoveCount(), 0, 124, 159, RGB(255, 1, 1));
		
		if (arrow) {
			Bit = (HBITMAP)SelectObject(memDC2, arrow);
			TransparentBlt(memDC1, getRect().left + width / 3, getRect().top - 30, 30, 50, memDC2, 0, 0, 816, 1083, RGB(255, 255, 255));
		}
	}
}

void Fish::AnimateBySpeed()
{
	float my_x = static_cast<float>(x);
	float my_y = static_cast<float>(y);
	float duration = 5;
	float dist = speed / duration;

	if (moveDir == 0b0000) // STOP
		return;

	// 대각선 이동을 위해 if문으로 계산
	if ((moveDir & MOVE_LEFT) == MOVE_LEFT) // LEFT
		my_x -= dist;
	if ((moveDir & MOVE_RIGHT) == MOVE_RIGHT) // RIGHT
		my_x += dist;
	if ((moveDir & MOVE_UP) == MOVE_UP) // UP
		my_y -= dist;
	if ((moveDir & MOVE_DOWN) == MOVE_DOWN) // DOWN
		my_y += dist;

	Move(static_cast<short>(my_x), static_cast<short>(my_y));
}
