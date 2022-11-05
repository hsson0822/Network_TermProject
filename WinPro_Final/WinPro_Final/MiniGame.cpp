#include "MiniGame.h"

std::random_device rd;
std::default_random_engine dre(rd());
std::uniform_int_distribution<> uid{ 100, 600 };

MiniGame::MiniGame() : fish(NULL), xFish(50), yFish(400), widthFish(100), heightFish(80), xBlock(900), speedBlock(30), sizeBlock(200), emptyBlockSize(200), emptyBlockPos(100), score(0), count(0), scoreBox({ 900, 0, 1000, 50 }), GameOver(false), wndRect({ 0,0,0,0 }), str(L"") {};

BOOL MiniGame::miniGamePaint(HDC hDC, HBITMAP f, HBITMAP bg, HBITMAP obs1, HBITMAP obs2)
{
	HDC memDC1, memDC2;
	HBITMAP hBit, oldBit1, oldBit2;
	hBit = CreateCompatibleBitmap(hDC, 1000, 800);
	memDC1 = CreateCompatibleDC(hDC);
	memDC2 = CreateCompatibleDC(memDC1);
	oldBit1 = (HBITMAP)SelectObject(memDC1, hBit);
	oldBit2 = (HBITMAP)SelectObject(memDC2, bg);
	StretchBlt(memDC1, 0, 0, 1000, 800, memDC2, 0, 0, 256, 192, SRCCOPY);
	SelectObject(memDC2, oldBit2);
	
	oldBit2 = (HBITMAP)SelectObject(memDC2, f);
	TransparentBlt(memDC1, xFish, yFish, widthFish, heightFish, memDC2, 124, 159, 124, 159, RGB(255, 1, 1));
	SelectObject(memDC2, oldBit2);

	oldBit2 = (HBITMAP)SelectObject(memDC2, obs2);
	TransparentBlt(memDC1, xBlock, 0, sizeBlock, emptyBlockPos, memDC2, 0, 0, 445, 640, RGB(255, 255, 255));
	SelectObject(memDC2, oldBit2);

	oldBit2 = (HBITMAP)SelectObject(memDC2, obs1);
	TransparentBlt(memDC1, xBlock, emptyBlockPos + emptyBlockSize, sizeBlock, wndRect.bottom - emptyBlockPos + emptyBlockSize, memDC2, 0, 0, 244, 468, RGB(255, 255, 255));
	/*Rectangle(memDC1, xBlock, 0, xBlock + sizeBlock, emptyBlockPos);
	Rectangle(memDC1, xBlock, emptyBlockPos + emptyBlockSize, xBlock + sizeBlock, 800);*/
	SelectObject(memDC2, oldBit2);
	wsprintf(str, L"Score : %d", showScore());
	DrawText(memDC1, str, lstrlen(str), &scoreBox, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

	BitBlt(hDC, 0, 0, 1000, 800, memDC1, 0, 0, SRCCOPY);
	SelectObject(memDC1, oldBit1);
	DeleteObject(hBit);
	DeleteDC(memDC1);
	DeleteDC(memDC2);

	if (GameOver) {
		return false;
	}
	return true;
}

void MiniGame::miniGameTimer()
{
	if (!GameOver) {
		yFish += 30;
		if (yFish + 30 > wndRect.bottom)
			yFish -= 30;
		xBlock -= speedBlock;

		if (xFish + widthFish > xBlock && xFish + widthFish < xBlock + sizeBlock) {
			if (check()) {
				score++;
			}
			else {
				miniGameEnd();
			}
		}

		if (xBlock + sizeBlock < 0) {
			xBlock = 950;
			emptyBlockPos = uid(dre);
			count++;
			if (count % 3 == 0)
				speedBlock += 3;
		}
	}
}

BOOL MiniGame::check()
{
	RECT temp;
	RECT fishRect = { xFish, yFish, xFish + widthFish, yFish + heightFish };
	RECT blockRect = { xBlock, emptyBlockPos, xBlock + sizeBlock, emptyBlockPos + emptyBlockSize };
	return IntersectRect(&temp, &fishRect, &blockRect);
}

void MiniGame::miniGameClick(POINT p)
{
	if (!GameOver)
		yFish -= 130;
	else {

	}
}

void MiniGame::miniGameEnd()
{
	GameOver = true;
}

int MiniGame::showScore() const
{
	return score;
}

void MiniGame::resetGame(RECT rect)
{
	xFish = 60;
	yFish = 400;
	widthFish = 100;
	heightFish = 80;
	xBlock = 900;
	speedBlock = 30;
	sizeBlock = 200;
	emptyBlockSize = 200;
	emptyBlockPos = 100;
	score = 0;
	count = 0;
	GameOver = false;
	wndRect = rect;
	scoreBox = { rect.right - 100, rect.top, rect.right, rect.top + 50 };
	wsprintf(str, L"");
}