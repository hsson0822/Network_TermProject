#pragma once
class Food
{
	int fishKinds;
	int x;
	int y;
	int width;
	int height;
	int moveCount;
	int maxAnimCount;
	int id;

public:
	Food(int k,int x, int y,int w, int h, int max, int id);

	int getX();
	void setX(int x);

	int getId() const;
	void setId(int i);

	int getY();
	void setY(int y);

	int getWidth();
	void setWidth(int w);

	int getHeight();
	void setHeight(int h);

	int getMoveCount();
	void addMoveCount();
	void resetMoveCount();

	int getMaxCount();

	int getFishKinds();
	void eraseFishKinds() { fishKinds = -1; }


};