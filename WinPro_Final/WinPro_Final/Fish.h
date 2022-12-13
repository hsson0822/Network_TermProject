#pragma once
#include <windows.h>
#include <chrono>
#include <string>

class Fish
{
	RECT rect;
	short x, y;
	int width, height;
	int state;
	int moveCount;
	unsigned char moveDir;
	BOOL goXY;
	BOOL goLR;
	BOOL goUD;
	bool is_active;
	int score;
	std::wstring text_score;
	double speed;
	std::chrono::system_clock::time_point last_move;

public:
	int is_caught;
	std::chrono::system_clock::time_point last_interpolation;

public:
	Fish();
	Fish(int posX, int posY);

	void Init();

	RECT getRect();
	void setRect(RECT r);

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
	short GetX() const { return x; };
	short GetY() const { return y; };
	bool GetIsActive() const { return is_active; }
	int GetScore() const { return score; }
	void SetScore(int val);

	void Draw(const HDC& memDC1, const HDC& memDC2, HBITMAP image, HBITMAP arrow);

	void AnimateBySpeed();

};