#pragma once
#include <windows.h>

class Fish
{
	RECT rect;
	short x, y;
	int width, height;
	int age;
	int exp;
	int maxExp;
	int state;
	int moveCount;
	int moveDir;
	BOOL goXY;
	BOOL goLR;
	BOOL goUD;

public:
	Fish(int posX, int posY);

	RECT getRect();
	void setRect(RECT r);

	int getAge();
	void addAge();

	int getExp();
	void setExp(int e);

	int getMaxExp();

	int getWidth();
	void setWidth(int w);

	int getHeight();
	void setHeight(int h);

	int getMoveCount();
	void addMoveCount();
	void resetMoveCount();

	int getMoveDir();
	void setMoveDir(int m);

	BOOL isXY();
	BOOL isLR();
	BOOL isUD();

	void setXY(bool xy);
	void setLR(bool lr);
	void setUD(bool ud);

	void Move(short x, short y);

	void SetX(short posX) { x = posX; }
	void SetY(short posY) { y = posY; }
	short GetX() const { return x; };
	short GetY() const { return y; };
};