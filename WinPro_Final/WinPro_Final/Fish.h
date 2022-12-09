#pragma once
#include <windows.h>
#include <chrono>

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
	unsigned char moveDir;
	BOOL goXY;
	BOOL goLR;
	BOOL goUD;
	bool is_active;
	int score;
	double speed;
	std::chrono::system_clock::time_point last_move;

public:
	int is_caught;
	std::chrono::system_clock::time_point last_interpolation;

public:
	Fish();
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

	unsigned char getMoveDir();
	void setMoveDir(unsigned char dir);

	double GetSpeed() const;
	void SetSpeed(double value);

	BOOL isXY();
	BOOL isLR();
	BOOL isUD();

	void setXY(bool xy);
	void setLR(bool lr);
	void setUD(bool ud);

	void Move(short x, short y);

	void SetX(short posX) { x = posX; }
	void SetY(short posY) { y = posY; }
	void SetIsActive(bool active) { is_active = active; }
	void setWH(int w, int h) { width = w; height = h; }
	void SetScore(int value) { score = value; }
	short GetX() const { return x; };
	short GetY() const { return y; };
	bool GetIsActive() const { return is_active; }
	int GetScore() const { return score; }
	
	void Draw(const HDC& memDC1, const HDC& memDC2, HBITMAP image, HBITMAP arrow);

	void AnimateBySpeed();
};