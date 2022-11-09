#pragma comment (lib, "msimg32.lib")
#pragma comment (lib, "winmm.lib")

#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <vector>
#include <mmsystem.h>
#include "resource.h"
#include "Fish.h"
#include "Food.h"

using namespace std;

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Growing Fish";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int GetExpPer(Fish& fish);
int CheckAge(int);

RECT rect;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, width, height - 100, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	HDC memDC1, memDC2;

	HBITMAP oldBit1, oldBit2;
	static HBITMAP hBitmap;
	static HBITMAP back1, back2, back3, back4, floor;
	static HBITMAP normalImage, angryImage, cryImage, happyImage;
	static HBITMAP foodButtonImage;
	static HBITMAP jelly, crab, squid;
	static HBITMAP net, hook, shark;

	static HBITMAP obs1, obs2;
	static HBRUSH hBrush, oldBrush;

	static BOOL isGameStart = false;

	static int selectBack;

	static Fish fish(0, 0);

	static vector<Food*> foods;
	static int foodKinds;
	static int foodCount;
	static int foodMax;
	static RECT foodButton;
	static int randX, randY;
	static POINT mousePoint;

	static BOOL caught;
	static int eventNum;
	static int eventClick;
	static BOOL eventOut;

	static RECT netRect;
	static int netDir;

	static int hookX;
	static RECT hookRect;
	static int hookCount;

	static RECT sharkRect;
	static int sharkDir;
	static int sharkCount;
	static int sharkY;
	static int sharkWave;

	static HBITMAP playButton;
	static RECT playButtonRect;

	static int angryCount;
	static int foodExp;
	static BOOL autoMode;

	switch (uMsg)
	{
	case WM_CREATE:
		srand((unsigned)time(NULL));
		GetClientRect(hWnd, &rect);

		back1 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK1));
		back2 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK2));
		back3 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK3));
		back4 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK4));
		floor = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FLOOR));
		normalImage = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NORMAL));
		angryImage = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_ANGRY));
		cryImage = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CRY));
		happyImage = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HAPPY));
		jelly = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_JELLY));
		crab = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CRAB));
		squid = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SQUID));
		net = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NET));
		hook = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HOOK));
		shark = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SHARK));
		foodButtonImage = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FOOD));

		obs1 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OBS));
		obs2 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OBS2));

		playButton = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PLAY));

		angryCount = 0;
		autoMode = FALSE;

		selectBack = 1;

		fish.setRect(RECT{ rect.right / 2 , rect.bottom / 2,(rect.right / 2) + fish.getWidth(),(rect.bottom / 2) + fish.getHeight() });
		foodButton = { rect.right - 100,rect.bottom - 100,rect.right,rect.bottom };
		foodCount = 0;
		foodMax = 20;

		playButtonRect = { rect.right / 2 - 100 , rect.bottom / 2 + 200 , rect.right / 2 + 100, rect.bottom / 2 + 300 };
		//playButtonRect = { rect.right - 100,rect.bottom - 100,rect.right,rect.bottom };

		caught = false;
		eventNum = 5;
		eventOut = false;

		sharkWave = 0;

		foodExp = 25;

		PlaySound(L"Aquarium.wav", NULL, SND_ASYNC | SND_LOOP);

		SetTimer(hWnd, 1, 70, NULL);	// 기본

		break;

	case WM_KEYDOWN:
		if (!isGameStart)
			break;
		switch (wParam)
		{
		case VK_LEFT:
			if (caught)
				break;
			fish.setXY(true);
			fish.setLR(false);
			fish.setMoveDir(0);
			break;
		case VK_RIGHT:
			if (caught)
				break;
			fish.setXY(true);
			fish.setLR(true);
			fish.setMoveDir(1);
			break;
		case VK_UP:
			if (caught)
				break;
			fish.setXY(false);
			fish.setUD(false);
			fish.setMoveDir(2);
			break;
		case VK_DOWN:
			if (caught)
				break;
			fish.setXY(false);
			fish.setUD(true);
			fish.setMoveDir(3);
			break;

		case VK_SPACE:
			if (foodCount < foodMax)
			{
				foodKinds = rand() % 3;
				randX = rand() % rect.right;
				randY = rand() % rect.bottom;
				if (foodKinds == 0)
				{
					//해파리
					foods.push_back(new Food(0, randX, randY, 27, 30, 4));
				}
				else if (foodKinds == 1)
				{
					//게
					foods.push_back(new Food(1, randX, randY, 85, 61, 2));
				}
				else
				{
					//오징어
					foods.push_back(new Food(2, randX, randY, 47, 72, 10));
				}


				++foodCount;
			}
			break;

		case VK_NUMPAD1:
			selectBack = 1;
			break;
		case VK_NUMPAD2:
			selectBack = 2;
			break;
		case VK_NUMPAD3:
			selectBack = 3;
			break;
		case VK_NUMPAD4:
			selectBack = 4;
			break;
		}
		break;

	case WM_KEYUP:
		if (!isGameStart)
			break;
		switch (wParam)
		{
		case VK_LEFT:
			fish.setXY(false);
			fish.setLR(false);
			fish.setMoveDir(4);
			break;
		case VK_RIGHT:
			fish.setXY(false);
			fish.setLR(true);
			fish.setMoveDir(4);
			break;
		case VK_UP:
		case VK_DOWN:
			if (caught)
				break;
			if (fish.isLR())
			{
				fish.setXY(false);
				fish.setLR(true);
			}
			else
			{
				fish.setXY(false);
				fish.setLR(false);
			}
			fish.setMoveDir(4);
			break;
		}

	case WM_LBUTTONDOWN:
		mousePoint = { LOWORD(lParam),HIWORD(lParam) };

		if (PtInRect(&playButtonRect, mousePoint))
		{
			isGameStart = true;

			SetTimer(hWnd, 2, 70, NULL);	// 먹이 낙하
			SetTimer(hWnd, 3, 70, NULL);	// 물고기 이동 / 먹이 섭취
			SetTimer(hWnd, 4, 20000, NULL);	// 이벤트 생성 20초
			SetTimer(hWnd, 5, 70, NULL);	// 이벤트 진행
		}

		if (!isGameStart)
			break;

		if (PtInRect(&foodButton, mousePoint))
		{
			if (foodCount < foodMax)
			{
				foodKinds = rand() % 3;
				randX = rand() % rect.right;
				randY = rand() % rect.bottom;
				if (foodKinds == 0)
				{
					//해파리
					foods.push_back(new Food(0, randX, randY, 27, 30, 4));
				}
				else if (foodKinds == 1)
				{
					//게
					foods.push_back(new Food(1, randX, randY, 85, 61, 2));
				}
				else
				{
					//오징어
					foods.push_back(new Food(2, randX, randY, 47, 72, 10));
				}


				++foodCount;
			}
		}

		if (PtInRect(&netRect, mousePoint))
		{
			if (eventClick > 5)
				break;

			++eventClick;
			if (eventClick == 5)
			{
				if (netDir == 0)
				{
					netDir = 1;
				}
				else
				{
					netDir = 0;
				}
				eventOut = true;
			}
		}

		if (PtInRect(&hookRect, mousePoint))
		{
			if (eventClick > 5)
				break;

			++eventClick;
			if (eventClick == 5)
			{
				hookCount = 201;
				eventOut = true;
			}
		}

		if (PtInRect(&sharkRect, mousePoint))
		{
			if (eventClick > 5)
				break;

			++eventClick;
			if (eventClick == 5)
			{
				if (sharkDir == 0)
				{
					sharkDir = 1;
				}
				else
				{
					sharkDir = 0;
				}
				eventOut = true;
			}
		}

		break;


	case WM_TIMER:
		switch (wParam)
		{
		case 1:
			hDC = GetDC(hWnd);

			if (hBitmap == NULL)
				hBitmap = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);

			memDC1 = CreateCompatibleDC(hDC);
			memDC2 = CreateCompatibleDC(memDC1);
			oldBit1 = (HBITMAP)SelectObject(memDC1, hBitmap);

			//배경화면
			if (selectBack == 1)
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, back1);
				StretchBlt(memDC1, 0, 0, rect.right, rect.bottom, memDC2, 0, 0, 256, 192, SRCCOPY);
			}
			else if (selectBack == 2)
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, back2);
				StretchBlt(memDC1, 0, 0, rect.right, rect.bottom, memDC2, 0, 0, 256, 192, SRCCOPY);
			}
			else if (selectBack == 3)
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, back3);
				StretchBlt(memDC1, 0, 0, rect.right, rect.bottom, memDC2, 0, 0, 256, 192, SRCCOPY);
			}
			else
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, back4);
				StretchBlt(memDC1, 0, 0, rect.right, rect.bottom, memDC2, 0, 0, 256, 192, SRCCOPY);
			}

			oldBit2 = (HBITMAP)SelectObject(memDC2, floor);
			TransparentBlt(memDC1, 0, rect.bottom - 80, rect.right, 84, memDC2, 0, 0, rect.right, 84, RGB(255, 1, 1));

			//물고기

			if (!caught)
			{
				if (!eventOut || angryCount > 30) { // 평상시
					oldBit2 = (HBITMAP)SelectObject(memDC2, normalImage);
					if (fish.isLR())
						TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
					else
						TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 0, 124, 159, RGB(255, 1, 1));
				}
				else if (eventOut) { // 이벤트 5회 클릭
					oldBit2 = (HBITMAP)SelectObject(memDC2, angryImage);
					if (fish.isLR())
						TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
					else
						TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 0, 124, 159, RGB(255, 1, 1));
					angryCount++;
				}
			}
			else
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, cryImage);
				if (fish.isLR())
					TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 159, 124, 159, RGB(255, 1, 1));
				else
					TransparentBlt(memDC1, fish.getRect().left, fish.getRect().top, fish.getWidth(), fish.getHeight(), memDC2, 124 * fish.getMoveCount(), 0, 124, 159, RGB(255, 1, 1));
			}
			fish.addMoveCount();
			if (fish.getMoveCount() > 3)
				fish.resetMoveCount();

			//먹이
			for (auto* f : foods)
			{
				//Ellipse(memDC1, f->getX(), f->getY(), f->getX() + 10, f->getY() + 10);
				if (f->getFishKinds() == 0)
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, jelly);
					TransparentBlt(memDC1, f->getX(), f->getY(), 30, 30, memDC2, 27 * f->getMoveCount(), 0, 27, 30, RGB(255, 1, 1));
				}
				else if (f->getFishKinds() == 1)
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, crab);
					TransparentBlt(memDC1, f->getX(), f->getY(), 40, 30, memDC2, 85 * f->getMoveCount(), 0, 85, 61, RGB(255, 1, 1));
				}
				else
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, squid);
					TransparentBlt(memDC1, f->getX(), f->getY(), 30, 50, memDC2, 47 * f->getMoveCount(), 0, 47, 72, RGB(255, 1, 1));
				}

				f->addMoveCount();
				if (f->getMoveCount() > f->getMaxCount() - 1)
					f->resetMoveCount();
			}

			//이벤트
			switch (eventNum)
			{
			case 0:
				oldBit2 = (HBITMAP)SelectObject(memDC2, net);
				//Rectangle(memDC1, netRect.left, netRect.top, netRect.right, netRect.bottom);
				if (netDir == 0)
					TransparentBlt(memDC1, netRect.left, netRect.top, 200, 400, memDC2, 1364, 0, 1364, 2438, RGB(255, 1, 1));
				else
					TransparentBlt(memDC1, netRect.left, netRect.top, 200, 400, memDC2, 0, 0, 1364, 2438, RGB(255, 1, 1));
				break;

			case 1:
				oldBit2 = (HBITMAP)SelectObject(memDC2, hook);
				TransparentBlt(memDC1, hookRect.left, hookRect.top, 100, 300, memDC2, 0, 0, 536, 1500, RGB(255, 1, 1));
				//Rectangle(memDC1, hookRect.left, hookRect.top, hookRect.right, hookRect.bottom);
				break;
			case 2:
				oldBit2 = (HBITMAP)SelectObject(memDC2, shark);
				if (sharkDir == 0)
					TransparentBlt(memDC1, sharkRect.left, sharkRect.top, 200, 100, memDC2, 97 * sharkCount, 34, 97, 34, RGB(255, 1, 1));
				else
					TransparentBlt(memDC1, sharkRect.left, sharkRect.top, 200, 100, memDC2, 97 * sharkCount, 0, 97, 34, RGB(255, 1, 1));

				++sharkCount;
				if (sharkCount > 2)
					sharkCount = 0;

				//Rectangle(memDC1, sharkRect.left, sharkRect.top, sharkRect.right, sharkRect.bottom);
				break;
			case 3:
				break;
			}

			if (caught)
			{
				if (fish.getRect().right < rect.left || fish.getRect().bottom < rect.top || fish.getRect().left > rect.right || fish.getRect().top > rect.bottom)
				{
					KillTimer(hWnd, 1);
					KillTimer(hWnd, 2);
					KillTimer(hWnd, 3);
					KillTimer(hWnd, 4);
					KillTimer(hWnd, 5);
					MessageBox(hWnd, L"개복치가 잡혀갔습니다. ㅠㅜ", L"Game Over", MB_OK);
					PlaySound(NULL, NULL, NULL);
					PostQuitMessage(0);
				}
			}

			if (!isGameStart)
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, playButton);
				TransparentBlt(memDC1, playButtonRect.left, playButtonRect.top, 200, 100, memDC2, 0, 0, 1271, 401, RGB(255, 255, 255));
				//Rectangle(memDC1, playButtonRect.left, playButtonRect.top, playButtonRect.right, playButtonRect.bottom);
			}
			else
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, foodButtonImage);
				//Rectangle(memDC1, foodButton.left, foodButton.top, foodButton.right, foodButton.bottom);
				TransparentBlt(memDC1, foodButton.left, foodButton.top, 100, 100, memDC2, 0, 0, 836, 834, RGB(255, 1, 1));
			}



			SelectObject(memDC2, oldBit2);
			DeleteObject(memDC2);
			SelectObject(memDC1, oldBit1);
			DeleteObject(memDC1);
			InvalidateRgn(hWnd, NULL, false);
			ReleaseDC(hWnd, hDC);
			break;

			//먹이 낙하
		case 2:
			for (vector<Food*>::iterator iter = foods.begin(); iter != foods.end(); ++iter)
			{
				(*iter)->setY((*iter)->getY() + 4);
				if ((*iter)->getY() > rect.bottom - 120)
				{
					iter = foods.erase(iter);
					--foodCount;
				}

				if (iter == foods.end())
					break;
			}

			break;

			//물고기 이동 및 먹이 섭취
		case 3:
			if (!caught)
			{
				if (fish.getMoveDir() == 0)
				{
					//왼쪽
					fish.setRect(RECT{ fish.getRect().left - 20,fish.getRect().top, fish.getRect().right - 20, fish.getRect().bottom });
					if (fish.getRect().left < rect.left)
						fish.setRect(RECT{ fish.getRect().left + 20,fish.getRect().top, fish.getRect().right + 20, fish.getRect().bottom });
				}
				else if (fish.getMoveDir() == 1)
				{
					//오른쪽
					fish.setRect(RECT{ fish.getRect().left + 20,fish.getRect().top, fish.getRect().right + 20, fish.getRect().bottom });
					if (fish.getRect().right > rect.right)
						fish.setRect(RECT{ fish.getRect().left - 20,fish.getRect().top, fish.getRect().right - 20, fish.getRect().bottom });
				}
				else if (fish.getMoveDir() == 2)
				{
					//위
					fish.setRect(RECT{ fish.getRect().left,fish.getRect().top - 20, fish.getRect().right, fish.getRect().bottom - 20 });
					if (fish.getRect().top < rect.top)
						fish.setRect(RECT{ fish.getRect().left,fish.getRect().top + 20, fish.getRect().right, fish.getRect().bottom + 20 });
				}
				else if (fish.getMoveDir() == 3)
				{
					//아래
					fish.setRect(RECT{ fish.getRect().left,fish.getRect().top + 20, fish.getRect().right, fish.getRect().bottom + 20 });
					if (fish.getRect().bottom > rect.bottom - 80)
						fish.setRect(RECT{ fish.getRect().left,fish.getRect().top - 20, fish.getRect().right, fish.getRect().bottom - 20 });
				}
			}
			else
			{
				if (eventNum == 0)
				{
					if (netDir == 0)
					{
						fish.setRect(RECT{ fish.getRect().left + 10,fish.getRect().top, fish.getRect().right + 10, fish.getRect().bottom });
					}
					else
					{
						fish.setRect(RECT{ fish.getRect().left - 10,fish.getRect().top, fish.getRect().right - 10, fish.getRect().bottom });
					}
				}
				else if (eventNum == 1)
				{
					fish.setRect(RECT{ fish.getRect().left,fish.getRect().top - 5, fish.getRect().right, fish.getRect().bottom - 5 });
				}
				else if (eventNum == 2)
				{
					if (sharkDir == 0)
						fish.setRect(RECT{ fish.getRect().left + 7,fish.getRect().top, fish.getRect().right + 7, fish.getRect().bottom });
					else
						fish.setRect(RECT{ fish.getRect().left - 7,fish.getRect().top, fish.getRect().right - 7, fish.getRect().bottom });
				}
			}

			if (!caught)
			{
				for (vector<Food*>::iterator iter = foods.begin(); iter != foods.end(); ++iter)
				{
					RECT temp;
					RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
					RECT foodRect = RECT{ (*iter)->getX(),(*iter)->getY(),(*iter)->getX() + (*iter)->getWidth(),(*iter)->getY() + (*iter)->getHeight() };
					if (IntersectRect(&temp, &fishRect, &foodRect))
					{
						iter = foods.erase(iter);
						--foodCount;
						fish.setExp(fish.getExp() + foodExp);

						if (fish.getExp() > fish.getMaxExp())
							fish.addAge();
					}

					if (iter == foods.end())
						break;
				}
				// 클리어
				if (fish.getAge() >= 30) {
					KillTimer(hWnd, 1);
					KillTimer(hWnd, 2);
					KillTimer(hWnd, 3);
					KillTimer(hWnd, 4);
					KillTimer(hWnd, 5);
					MessageBox(hWnd, L"개복치가 성장이 완료되었습니다!", L"Congratulations", MB_OK);
					PlaySound(NULL, NULL, NULL);
					PostQuitMessage(0);
				}
			}

			break;

			// 이벤트 생성
		case 4:
			eventNum = rand() % 3;
			//eventNum = 0;
			eventClick = 0;
			eventOut = false;
			// 화난 얼굴
			angryCount = 0;

			if (eventNum == 0)
			{
				netDir = rand() % 2;
				if (netDir == 0)
				{
					netRect = { rect.left - 200, rect.top, rect.left, rect.top + 400 };
				}
				else
				{
					netRect = { rect.right, rect.top, rect.right + 200, rect.top + 400 };
				}
			}
			else if (eventNum == 1)
			{
				hookX = rand() % rect.right;
				hookCount = 0;
				hookRect = { hookX,-300,hookX + 100,0 };
			}
			else if (eventNum == 2)
			{
				sharkDir = rand() % 2;
				sharkY = rand() % rect.bottom;
				if (sharkDir == 0)
					sharkRect = { rect.left - 200, sharkY,rect.left,sharkY + 100 };
				else
					sharkRect = { rect.right, sharkY,rect.right + 200,sharkY + 100 };
			}

			break;

			// 이벤트 재생
		case 5:
			switch (eventNum)
			{
				//그물
			case 0:
				if (netDir == 0)
				{
					netRect = { netRect.left + 10, netRect.top, netRect.right + 10,netRect.bottom };
				}
				else
				{
					netRect = { netRect.left - 10, netRect.top, netRect.right - 10,netRect.bottom };
				}

				if (!caught && !eventOut)
				{
					RECT temp;
					RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
					if (IntersectRect(&temp, &netRect, &fishRect))
					{
						caught = true;
					}
				}
				break;

				//낚시 바늘
			case 1:
				if (hookCount < 60)
				{
					hookRect = { hookRect.left,hookRect.top + 5 ,hookRect.right, hookRect.bottom + 5 };
				}
				else if (hookCount > 200)
				{
					hookRect = { hookRect.left,hookRect.top - 5 ,hookRect.right,hookRect.bottom - 5 };
				}
				++hookCount;

				if (!caught && !eventOut)
				{
					RECT temp;
					RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
					RECT hookR = RECT{ hookRect.left,hookRect.top + 240,hookRect.right - 40,hookRect.bottom };
					if (IntersectRect(&temp, &hookR, &fishRect))
					{
						caught = true;
						hookCount = 201;
					}
				}
				break;

				//상어
			case 2:
				if (sharkDir == 0)
				{
					sharkRect = { sharkRect.left + 7, sharkRect.top, sharkRect.right + 7,sharkRect.bottom };
				}
				else
				{
					sharkRect = { sharkRect.left - 7, sharkRect.top, sharkRect.right - 7,sharkRect.bottom };
				}

				++sharkWave;
				if (sharkWave < 12)
				{
					sharkRect = { sharkRect.left, sharkRect.top + 2, sharkRect.right,sharkRect.bottom + 2 };
					if (caught)
						fish.setRect(RECT{ fish.getRect().left,fish.getRect().top + 2, fish.getRect().right, fish.getRect().bottom + 2 });
				}
				else if (sharkWave < 24)
				{
					sharkRect = { sharkRect.left, sharkRect.top - 2, sharkRect.right,sharkRect.bottom - 2 };
					if (caught)
						fish.setRect(RECT{ fish.getRect().left,fish.getRect().top - 2, fish.getRect().right, fish.getRect().bottom - 2 });
				}
				else
					sharkWave = 0;

				if (!caught && !eventOut)
				{
					RECT temp;
					RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
					if (IntersectRect(&temp, &sharkRect, &fishRect))
					{
						caught = true;
					}
				}
				break;

			case 3:
				break;

			default:
				break;
			}
			break;
		case 7:
			if (foodCount < foodMax)
			{
				foodKinds = rand() % 3;
				randX = rand() % rect.right;
				randY = rand() % rect.bottom;
				if (foodKinds == 0)
				{
					//해파리
					foods.push_back(new Food(0, randX, randY, 27, 30, 4));
				}
				else if (foodKinds == 1)
				{
					//게
					foods.push_back(new Food(1, randX, randY, 85, 61, 2));
				}
				else
				{
					//오징어
					foods.push_back(new Food(2, randX, randY, 47, 72, 10));
				}


				++foodCount;
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_MODE_EASY:
			foodExp = 100;
			break;
		case ID_MODE_NORMAL:
			foodExp = 25;
			break;
		case ID_MODE_HARD:
			foodExp = 10;
			break;
		case ID_BACKGROUND_BACK1:
			selectBack = 1;
			break;
		case ID_BACKGROUND_BACK2:
			selectBack = 2;
			break;
		case ID_BACKGROUND_BACK3:
			selectBack = 3;
			break;
		case ID_BACKGROUND_BACK4:
			selectBack = 4;
			break;
		}
		break;

	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		// Growing Fish

		memDC1 = CreateCompatibleDC(hDC);
		oldBit1 = (HBITMAP)SelectObject(memDC1, hBitmap);

		memDC2 = CreateCompatibleDC(memDC1);

		BitBlt(hDC, 0, 0, rect.right, rect.bottom, memDC1, 0, 0, SRCCOPY);
		SelectObject(memDC1, oldBit1);
		DeleteObject(memDC2);
		DeleteObject(memDC1);

		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PlaySound(NULL, NULL, NULL);
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int GetExpPer(Fish& fish)
{
	int now = fish.getExp();
	int target = fish.getMaxExp();
	if (fish.getAge() == 0) {
		if (now < 10)
			return 0;
		else if (now < 20)
			return 2;
		else
			return 4;
	}
	if (100 - (target - now) < 25)
		return 0;
	else if (100 - (target - now) < 50)
		return 1;
	else if (100 - (target - now) < 75)
		return 2;
	else if (100 - (target - now) < 100)
		return 3;
	else
		return 4;
}

int CheckAge(int age)
{
	switch (age)
	{
	case 1:
		return 0;
	case 2:
		return 280;
	case 3:
		return 600;
	case 4:
		return 930;
	case 5:
		return 1270;
	case 6: return 1600;
	case 7: return 1900;
	case 8: return 2230;
	case 9: return 2550;
	case 0: return 2870;
	}
}

