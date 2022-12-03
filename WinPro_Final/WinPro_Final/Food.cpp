#include "Food.h"

Food::Food(int k, int x, int y, int w, int h, int m, int id) :fishKinds{ k }, x { x }, y{ y }, width{ w }, height{ h }, moveCount{ 0 }, maxAnimCount{ m }, id{-1}
{
}

int Food::getX() { return x; }
void Food::setX(int x) { this->x = x; }

int Food::getY() { return y; }
void Food::setY(int y) { this->y = y; }


int Food::getWidth() { return width; }
void Food::setWidth(int w) { width = w; }

int Food::getHeight() { return height; }
void Food::setHeight(int h) { height = h; }

int Food::getMoveCount() { return moveCount; }
void Food::addMoveCount() { ++moveCount; }
void Food::resetMoveCount() { moveCount = 0; }

int Food::getMaxCount() { return maxAnimCount;}

int Food::getFishKinds() { return fishKinds; }

int Food::getId() { return id; }
void Food::setId(int i) { id = i; }
