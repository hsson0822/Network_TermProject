#include "Fish.h"
#include "../../Network_TermProject_Server/Network_TermProject_Server/protocol.h"

Fish::Fish()
{
	width = FISH_INIT_WIDTH;
	height = FISH_INIT_HEIGHT;
	rect = { 0,0,0 + width, 0 + height };
	moveDir = 0;
	x = 0;
	y = 0;
	is_active = false;
	is_caught = -1;
	speed = FISH_INIT_SPEED;
	last_move = std::chrono::system_clock::now();
	last_interpolation = last_move;
	score = 0;
	text_score = std::to_wstring(score);
}

Fish::Fish(int posX, int posY) : Fish()
{
	rect = { posX,posY,posX + width, posY + height };
}

RECT Fish::getRect() { return rect; }
void Fish::setRect(RECT r) { rect = r; }

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

double Fish::GetSpeed() const { return speed; }
void Fish::SetSpeed(double value) { speed = value; }

BOOL Fish::isXY() { return goXY; }
BOOL Fish::isLR() { return goLR; }
BOOL Fish::isUD() { return goUD; }

void Fish::setXY(bool xy) { goXY = xy; }
void Fish::setLR(bool lr) { goLR = lr; }
void Fish::setUD(bool ud) { goUD = ud; }

void Fish::Move(short posX, short posY)
{
	x = posX;
	y = posY;
	setRect(RECT{ x, y, x + width, y + height });
}

void Fish::SetScore(int val)
{
	score = val;
	text_score = std::to_wstring(score);
}

void Fish::Draw(const HDC& memDC1, const HDC& memDC2, HBITMAP image, HBITMAP arrow)
{
	if (is_active) {
		HBITMAP Bit = (HBITMAP)SelectObject(memDC2, image);
		if (isLR())
			TransparentBlt(memDC1, x, y, width, height, memDC2, 124 * getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
		else
			TransparentBlt(memDC1, x, y, width, height, memDC2, 124 * getMoveCount(), 0, 124, 159, RGB(255, 1, 1));

		if (score > 0) {
			SetBkMode(memDC1, TRANSPARENT);
			TextOut(memDC1, x + (width / 2) - 20, y + height + 10, text_score.c_str(), text_score.size());
		}

		if (arrow) {
			Bit = (HBITMAP)SelectObject(memDC2, arrow);
			TransparentBlt(memDC1, x + width / 3, y - 30, 30, 50, memDC2, 0, 0, 816, 1083, RGB(255, 255, 255));
		}
	}
}

void Fish::AnimateBySpeed()
{
	float my_x = static_cast<float>(x);
	float my_y = static_cast<float>(y);
	float duration = MOVE_BIAS;

	std::chrono::system_clock::time_point cur = std::chrono::system_clock::now();
	auto exec = std::chrono::duration_cast<std::chrono::milliseconds>(cur - last_move).count();
	last_move = cur;

	float dist = speed / duration * exec;

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
