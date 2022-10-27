#pragma once
#include <Windows.h>
#include <random>
#include <tchar.h>

class MiniGame
{
	HBITMAP fish;
	int xFish;
	int yFish;
	int widthFish;
	int heightFish;
	int xBlock;
	int speedBlock;
	int sizeBlock;
	int emptyBlockSize;
	int emptyBlockPos;
	int score;
	int count;
	RECT scoreBox;
	BOOL GameOver;
	RECT wndRect;
	TCHAR str[20];

public:
	MiniGame();
	BOOL miniGamePaint(HDC, HBITMAP, HBITMAP, HBITMAP, HBITMAP);
	void miniGameTimer();
	BOOL check();			// 통과했는지 확인
	void miniGameClick(POINT);
	void miniGameEnd();
	int showScore() const;
	void resetGame(RECT);
};