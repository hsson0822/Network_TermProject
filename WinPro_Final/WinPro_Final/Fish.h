#pragma once
#include <windows.h>

class Fish
{
	RECT rect;
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
	Fish(int x, int y);

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
};