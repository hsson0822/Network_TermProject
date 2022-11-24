#include "Fish.h"

Fish::Fish()
{
	width = 120;
	height = 140;
	age = 0;
	exp = 0;
	maxExp = 20;
	rect = { 0,0,0 + width, 0 + height };
	moveDir = 4;
	x = 0;
	y = 0;
	is_active = false;
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
	++age;
	maxExp = age * 100 + 20;
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

int Fish::getMoveDir() { return moveDir;}
void Fish::setMoveDir(int m) { moveDir = m; }


BOOL Fish::isXY() { return goXY; }
BOOL Fish::isLR() { return goLR; }
BOOL Fish::isUD() { return goUD; }

void Fish::setXY(bool xy) { goXY = xy; }
void Fish::setLR(bool lr) { goLR = lr; }
void Fish::setUD(bool ud) { goUD = ud; }

void Fish::Move(short posX, short posY)
{
	if (posX > x)
		goLR = true;
	else if(posX < x)
		goLR = false;

	x = posX;
	y = posY;
	setRect(RECT{ x, y, x + width, y + height });
}

void Fish::Draw(const HDC& memDC1, const HDC& memDC2)
{
	if (is_active) {
		if (isLR())
			TransparentBlt(memDC1, getRect().left, getRect().top, getWidth(), getHeight(), memDC2, 124 * getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
		else
			TransparentBlt(memDC1, getRect().left, getRect().top, getWidth(), getHeight(), memDC2, 124 * getMoveCount(), 0, 124, 159, RGB(255, 1, 1));
	}
}