#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "msimg32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment(lib,"ws2_32")

// 콘솔 띄우기 명령줄, 코드 작성 완료시 삭제해야 함
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#include <iostream>
// -----------------------------------------

#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <vector>
#include <mmsystem.h>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include "resource.h"
#include "Fish.h"
#include "Food.h"
#include "../../Network_TermProject_Server/Network_TermProject_Server/protocol.h"

using namespace std;

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Growing Fish";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
int GetExpPer(Fish& fish);
int CheckAge(int);

RECT rect;

HWND hWnd;
HANDLE hThread;
SOCKET sock;
WSADATA wsa;
int retval;

// 자신의 id 구분
int id;
BOOL isReady = false;
BOOL isGameStart = false;
long start_x{ -300 };
RECT playButtonRect;

Fish fish(0, 0);
int fishSpeed = 5;
Fish players[MAX_USER];

RECT netRect;
int netDir;

RECT hookRect;
int hookX;
int hookCount;

RECT sharkRect;
int sharkDir;
int sharkCount;
int sharkY;
//int sharkWave;

int eventNum;


// players 는 다른 플레이어
// 현재는 Fish class를 사용하지만 불필요한 멤버 함수, 변수를 제외한 클래스로 배열을 만들어야 함

void SetFishRect(Fish& fish, const LONG x, const LONG y);

const char* SERVERIP = nullptr;

// 오류 검사용 함수
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	TCHAR message[100];
	wsprintf(message, L"[%s] %s\n", msg, (char*)lpMsgBuf);
	MessageBox(hWnd, message, L"오류", MB_OK);
	LocalFree(lpMsgBuf);
}

// 한번에 여러 패킷이 왔을 때 처리
// 패킷 하나를 처리하고 남아있다면 버퍼의 내용을 앞으로 당기고
// 처리한 만큼 남은 패킷의 크기를 감소
void overload_packet_process(char* buf, int packet_size, int& remain_packet)
{
	remain_packet -= packet_size;
	if (remain_packet > 0) {
		memcpy(buf, buf + packet_size, BUF_SIZE - packet_size);
	}
}

static vector<Food*> foods;
int col_x, col_y;


// 클라이언트에서 네트워크 통신용 쓰레드
DWORD WINAPI NetworkThread(LPVOID arg)
{
	int retval;
	SOCKET sock = reinterpret_cast<SOCKET>(arg);

	int len{};
	char buf[BUF_SIZE];
	char send_buf[BUF_SIZE];

	int remain_packet{};

	// 접속 후 바로 로그인 패킷을 보냄
	CS_LOGIN_PACKET packet;
	packet.type = CS_LOGIN;

	ZeroMemory(send_buf, BUF_SIZE);
	memcpy(send_buf, &packet, sizeof(packet));

	retval = send(sock, send_buf, sizeof(packet), 0);
	if (retval == SOCKET_ERROR) err_display("send()");
	// 본인 + 다른 플레이어	배열 초기화를 위해 패킷 전송 및 받기


	while (true) {
		retval = recv(sock, buf, BUF_SIZE, 0);
		if (SOCKET_ERROR == retval) {
			err_display("file size recv()");
			return 0;
		}
		else if (0 == retval) {
			return 0;
		}

		remain_packet = retval;
		// 남아있는 패킷 처리
		while (remain_packet > 0) {

			//패킷별 처리
			switch (buf[0]) {
			case SC_CREATE_OBSTACLE:
			{
				SC_CREATE_OBJCET_PACKET* packet = reinterpret_cast<SC_CREATE_OBJCET_PACKET*>(buf);

				eventNum = packet->object.type;
				short x = packet->object.pos.x;
				short y = packet->object.pos.y;
				col_x = packet->col_x;
				col_y = packet->col_y;


				switch (eventNum)
				{
				case NET:
					netDir = packet->dir;
					netRect = { x, y, x + col_x, y + col_y };
					break;

				case HOOK:
					hookRect = { x, y, x + HOOK_WIDTH, y + HOOK_HEIGHT };
					break;

				case SHARK:
					sharkDir = packet->dir;
					sharkRect = { x, y, x + SHARK_WIDTH, y + SHARK_HEIGHT };
					break;
				}

				overload_packet_process(buf, sizeof(SC_CREATE_OBJCET_PACKET), remain_packet);
				break;
			}

			case SC_CREATE_FOOD:
			{
				SC_CREATE_OBJCET_PACKET* packet = reinterpret_cast<SC_CREATE_OBJCET_PACKET*>(buf);

				short x = packet->object.pos.x;
				short y = packet->object.pos.y;
				switch (packet->object.type) {
				case JELLYFISH:
					foods.push_back(new Food(JELLYFISH, x, y, JELLYFISH_WIDTH, JELLYFISH_HEIGHT, 4, packet->index));
					break;

				case CRAB:
					foods.push_back(new Food(CRAB, x, y, CRAB_WIDTH, CRAB_HEIGHT, 2, packet->index));
					break;

				case SQUID:
					foods.push_back(new Food(SQUID, x, y, SQUID_WIDTH, SQUID_HEIGHT, 10, packet->index));
					break;

				}

				overload_packet_process(buf, sizeof(SC_CREATE_OBJCET_PACKET), remain_packet);
				break;
			}
			case SC_ERASE_FOOD:
			{
				SC_ERASE_OBJECT_PACKET* packet = reinterpret_cast<SC_ERASE_OBJECT_PACKET*>(buf);
				int id = packet->index;
				
				for (vector<Food*>::iterator iter = foods.begin(); iter != foods.end(); ++iter)
				{
					if ((*iter)->getId() == packet->index)
					{
						(*iter)->eraseFishKinds();
					}

					if (iter == foods.end())
						break;
				}

				overload_packet_process(buf, sizeof(SC_ERASE_OBJECT_PACKET), remain_packet);
				break;
			}
			case SC_ERASE_OBSTACLE:
			{
				SC_ERASE_OBJECT_PACKET* packet = reinterpret_cast<SC_ERASE_OBJECT_PACKET*>(buf);
				eventNum = -1;

				overload_packet_process(buf, sizeof(SC_ERASE_OBJECT_PACKET), remain_packet);
				break;
			}
			case SC_UPDATE_OBSTACLE:
			{
				SC_UPDATE_OBJECT_PACKET* packet = reinterpret_cast<SC_UPDATE_OBJECT_PACKET*>(buf);

				short x = packet->oi.pos.x;
				short y = packet->oi.pos.y;


				switch (packet->oi.type)
				{
				case NET:
					netRect = { x, y, x + NET_WIDTH, y + NET_HEIGHT };
					break;
				case SHARK:
					sharkRect = { x, y, x + SHARK_WIDTH, y + SHARK_HEIGHT };
					break;
				case HOOK:
					hookRect = { x, y, x + HOOK_WIDTH, y + HOOK_HEIGHT };
					break;
				}

				overload_packet_process(buf, sizeof(SC_UPDATE_OBJECT_PACKET), remain_packet);
				break;
			}

			case SC_PLAYER_DEAD: {
				SC_DEAD_PACKET* packet = reinterpret_cast<SC_DEAD_PACKET*>(buf);

				int other_id = packet->id;
				if (id == other_id) {
					fish.Move(packet->x, packet->y);
					fish.SetScore(packet->score);
				}
				else {
					players[other_id].Move(packet->x, packet->y);
					players[other_id].SetScore(packet->score);
				}

				overload_packet_process(buf, sizeof(SC_DEAD_PACKET), remain_packet);
				break;
			}

			case SC_UPDATE_PLAYER_WH:
			{
				SC_UPDATE_PLAYER_PACKET* packet = reinterpret_cast<SC_UPDATE_PLAYER_PACKET*>(buf);

				short w = packet->w;
				short h = packet->h;

				if (id == packet->id)
				{
					fish.setWH(packet->w, packet->h);
					fish.is_caught = packet->is_caught;
					fish.SetScore(packet->score);
					if (-1 != fish.is_caught)
						fish.setMoveDir(0);
				
				}
				else {
					players[packet->id].setWH(packet->w, packet->h);
					players[packet->id].SetScore(packet->score);
					players[packet->id].is_caught = packet->is_caught;
					if (-1 != players[packet->id].is_caught)
						players[packet->id].setMoveDir(0);

				}

				overload_packet_process(buf, sizeof(SC_UPDATE_PLAYER_PACKET), remain_packet);
				break;
			}

			case SC_ADD_PLAYER: {
				// 다른 플레이어 추가
				// 다른 플레이어 active 로 변경
				SC_ADD_PLAYER_PACKET* packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(buf);
				int other_id = packet->id;

				SetFishRect(players[other_id], rect.right / 2 + start_x, rect.bottom / 2);
				start_x += 200;
				players[other_id].SetIsActive(true);

				overload_packet_process(buf, sizeof(SC_ADD_PLAYER_PACKET), remain_packet);
				break;
			}

			case SC_LEAVE_PLAYER: {
				SC_LEAVE_PLAYER_PACKET* packet = reinterpret_cast<SC_LEAVE_PLAYER_PACKET*>(buf);
				int other_id = packet->id;

				players[other_id].SetIsActive(false);

				overload_packet_process(buf, sizeof(SC_LEAVE_PLAYER_PACKET), remain_packet);
				break;
			}

			case SC_LOGIN_OK: {
				// 본인의 id 기록
				// 클라이언트에서 자신 + 다른 플레이어를 그릴 때
				// 본인의 id를 통해 구분하면 됨
				{
					SC_LOGIN_OK_PACKET* packet = reinterpret_cast<SC_LOGIN_OK_PACKET*>(buf);
					id = packet->id;
				}


				CS_READY_PACKET packet;
				packet.type = CS_PLAYER_READY;
				packet.id = id;

				ZeroMemory(send_buf, BUF_SIZE);
				memcpy(send_buf, &packet, sizeof(packet));

				retval = send(sock, send_buf, sizeof(packet), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
				}
				overload_packet_process(buf, sizeof(SC_LOGIN_OK_PACKET), remain_packet);
				break;
			}

			case SC_GAME_START: {
				SC_GAME_START_PACKET* packet = reinterpret_cast<SC_GAME_START_PACKET*>(buf);

				for (int i = 0; i < MAX_USER; ++i) {
					if (id == i) {
						fish.SetX(packet->pos[i].x);
						fish.SetY(packet->pos[i].y);
						fish.Move(packet->pos[i].x, packet->pos[i].y);
					}
					else {
						players[i].SetX(packet->pos[i].x);
						players[i].SetY(packet->pos[i].y);
						players[i].Move(packet->pos[i].x, packet->pos[i].y);
					}
				}

				isReady = false;
				isGameStart = true;
				overload_packet_process(buf, sizeof(SC_GAME_START_PACKET), remain_packet);
				//SetTimer(hWnd, 2, 70, NULL);	// 먹이 낙하
				SetTimer(hWnd, 3, MOVE_COOLTIME, NULL);	// 물고기 이동 / 먹이 섭취
				//SetTimer(hWnd, 4, 30000, NULL);	// 이벤트 생성 30초
				//SetTimer(hWnd, 5, 70, NULL);	// 이벤트 진행

				break;
			}

			case SC_CHANGE_DIRECTION: {
				SC_CHANGE_DIRECTION_PACKET* packet = reinterpret_cast<SC_CHANGE_DIRECTION_PACKET*>(buf);

				// -----------------------------
				// id로 나 or 플레이어 이동 처리
				// packet->id ~~
				// --------------------------
				int other_id = packet->id;

				if (other_id == id) {
					fish.SetSpeed(packet->speed);
				}
				else {
					players[other_id].setMoveDir(packet->dir);
					players[other_id].SetSpeed(packet->speed);
				}

				overload_packet_process(buf, sizeof(SC_CHANGE_DIRECTION_PACKET), remain_packet);
				break;
			}

			case SC_INTERPOLATION: {
				SC_INTERPOLATION_PACKET* packet = reinterpret_cast<SC_INTERPOLATION_PACKET*>(buf);
				int other_id = packet->id;
				
				if (other_id == id) {
					fish.Move(packet->x, packet->y);
				}
				else {
					players[other_id].Move(packet->x, packet->y);
				}

				overload_packet_process(buf, sizeof(SC_INTERPOLATION_PACKET), remain_packet);
				break;
			}

			case SC_CAUGHT: {
				SC_CAUGHT_PACKET* packet = reinterpret_cast<SC_CAUGHT_PACKET*>(buf);

				int other_id = packet->id;
				if (other_id == id) {
					fish.Move(packet->x, packet->y);
				}
				else {
					players[other_id].Move(packet->x, packet->y);
				}


				overload_packet_process(buf, sizeof(SC_CAUGHT_PACKET), remain_packet);
				break;
			}


			case SC_GAME_OVER: {

				SC_GAME_OVER_PACKET* packet = reinterpret_cast<SC_GAME_OVER_PACKET*>(buf);
				unordered_map<int, int> map;

				// 게임 종료시 점수를 마지막으로 받음
				for (int i = 0; i < MAX_USER; ++i) {
					map[i] = packet->scores[i];
				}


				vector<pair<int, int>> v(map.begin(), map.end());
				sort(v.begin(), v.end(), [](pair<int, int>& a, pair<int, int>& b)
					{
						return a.second > b.second;
					});

				CS_DISCONNECT_PACKET send_packet;
				send_packet.type = CS_DISCONNECT;

				ZeroMemory(send_buf, BUF_SIZE);
				memcpy(send_buf, &send_packet, sizeof(send_packet));

				//KillTimer(hWnd, 1);
				KillTimer(hWnd, 2);

				retval = send(sock, send_buf, sizeof(send_packet), 0);
				if (retval == SOCKET_ERROR) err_display("disconnect game()");

				TCHAR message[100];
				wsprintf(message, L"%d번 플레이어 : %d점\n%d번 플레이어 : %d점\n%d번 플레이어 : %d점",v[0].first, v[0].second, v[1].first, v[1].second, v[2].first, v[2].second);
				MessageBox(hWnd, message, L"게임 종료", MB_OK);

				foods.clear();

				hookRect = { 0,0,0,0 };
				sharkRect = { 0,0,0,0 };
				netRect = { 0,0,0,0 };

				isGameStart = false;
				isReady = false;

				SetFishRect(fish, rect.right / 2 - 300, rect.bottom / 2);

				for (int i = 0; i < MAX_USER; ++i)
					players[i].SetIsActive(false);

				playButtonRect = { rect.right / 2 - 100 , rect.bottom / 2 + 200 , rect.right / 2 + 100, rect.bottom / 2 + 300 };

				start_x = -100;

				overload_packet_process(buf, sizeof(SC_GAME_OVER_PACKET), remain_packet);
				break;
			}


			}
			// 패킷 별 처리

		}
		// 여러 패킷이 한번에 왔을 때 처리

	}

	closesocket(sock);

	return 0;

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
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

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 1800, 900, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	ifstream in;
	in.open("ipaddress.txt");

	std::string s;
	if (!in.is_open()) {
		TCHAR message[100];
		wsprintf(message, L"ipaddress.txt 파일이 없습니다!");
		MessageBox(hWnd, message, L"오류", MB_OK);
		return 0;
	}
	
	in >> s;
	SERVERIP = s.c_str();


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
	static HBITMAP back1;
	static HBITMAP normalImage, angryImage, cryImage, happyImage;
	static HBITMAP foodButtonImage;
	static HBITMAP jelly, crab, squid;
	static HBITMAP net, hook, shark;

	static HBITMAP obs1, obs2;
	static HBRUSH hBrush, oldBrush;

	static int selectBack;

	static HBITMAP arrow;

	static int foodKinds;
	static int foodCount;
	static int foodMax;
	static RECT foodButton;
	static int randX, randY;
	static POINT mousePoint;

	static int eventClick;
	static BOOL eventOut;



	static HBITMAP playButton;


	static HBITMAP loading;
	static RECT loadingRect;
	static int loadingCount;

	static int angryCount;
	static int foodExp;
	static BOOL autoMode;

	switch (uMsg)
	{
	case WM_CREATE:
		srand((unsigned)time(NULL));
		GetClientRect(hWnd, &rect);

		back1 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACK1));
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
		arrow = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_ARROW));

		obs1 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OBS));
		obs2 = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_OBS2));

		playButton = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PLAY));
		loading = (HBITMAP)LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_LOADING));

		angryCount = 0;
		autoMode = FALSE;

		selectBack = 1;

		SetFishRect(fish, rect.right / 2 + start_x, rect.bottom / 2);
		start_x += 200;
		fish.SetIsActive(true);
		foodButton = { rect.right - 100,rect.bottom - 100,rect.right,rect.bottom };
		foodCount = 0;
		foodMax = 20;

		playButtonRect = { rect.right / 2 - 100 , rect.bottom / 2 + 200 , rect.right / 2 + 100, rect.bottom / 2 + 300 };

		loadingRect = { rect.right / 2 - 50 , rect.bottom / 2 + 200 , rect.right / 2 + 100, rect.bottom / 2 + 300 };
		loadingCount = 0;

		eventNum = 5;
		eventOut = false;

		//sharkWave = 0;

		foodExp = 25;


		PlaySound(L"Aquarium.wav", NULL, SND_ASYNC | SND_LOOP);

		SetTimer(hWnd, 1, 70, NULL);	// 기본

		break;

	case WM_KEYDOWN:
	{
		if (!isGameStart)
			break;

		if (fish.is_caught != -1)
			break;

		unsigned char dir = 0b0000;
		if (GetKeyState('A') & 0x8000) {		// LEFT
			dir = (dir | MOVE_LEFT);
		}
		if (GetKeyState('D') & 0x8000) {		// RIGHT
			dir = (dir | MOVE_RIGHT);
		}
		if (GetKeyState('W') & 0x8000) {		// UP
			dir = (dir | MOVE_UP);
		}
		if (GetKeyState('S') & 0x8000) {		// DOWN
			dir = (dir | MOVE_DOWN);
		}

		// 이동키를 눌렀다면
		if (dir != 0b0000) {
			// 기존 이동 방향과 키입력 방향이 같다면 빠져나오도록 함
			// 방향 전환시 한번만 패킷전송을 하기 위해
			unsigned char direction = fish.getMoveDir();
			if (direction == dir)
				break;

			fish.setMoveDir(dir);

			CS_CHANGE_DIRECTION_PACKET packet;
			packet.type = CS_CHANGE_DIRECTION;
			packet.dir = dir;

			char send_buf[BUF_SIZE];
			ZeroMemory(send_buf, BUF_SIZE);
			memcpy(send_buf, &packet, sizeof(packet));


			retval = send(sock, send_buf, sizeof(packet), 0);
			if (retval == SOCKET_ERROR) err_display("move key down send()");
		}


		break;
	}

	case WM_KEYUP: {
		if (!isGameStart)
			break;

		if (fish.is_caught != -1)
			break;

		unsigned char dir = 0b0000;

		switch (wParam)
		{
		case 'A':
			dir = MOVE_LEFT;
			break;
		case 'D':
			dir = MOVE_RIGHT;
			break;
		case 'W':
			dir = MOVE_UP;
			break;
		case 'S':
			dir = MOVE_DOWN;
			break;
		}

		if (dir != 0b0000) {
			// 현재 방향에서 키를 뗀 방향을 xor하면 남은 이동방향이 나옴
			unsigned char direction = fish.getMoveDir();
			direction = (direction ^ dir);
			fish.setMoveDir(direction);

			// 이동방향이 0이 아님 = 대각선으로 움직이다 한쪽방향을 뗌
			CS_CHANGE_DIRECTION_PACKET packet;
			packet.type = CS_CHANGE_DIRECTION;
			packet.dir = direction;

			char send_buf[BUF_SIZE];
			ZeroMemory(send_buf, BUF_SIZE);
			memcpy(send_buf, &packet, sizeof(packet));


			retval = send(sock, send_buf, sizeof(packet), 0);
			if (retval == SOCKET_ERROR) err_display("move key down send()");
			

			// 이동방향이 0 = 모든 이동키를 뗌
			if(direction == 0){
				CS_INTERPOLATION_PACKET packet;
				packet.type = CS_INTERPOLATION;
				packet.x = fish.GetX();
				packet.y = fish.GetY();

				char send_buf[BUF_SIZE];
				ZeroMemory(send_buf, BUF_SIZE);
				memcpy(send_buf, &packet, sizeof(packet));


				retval = send(sock, send_buf, sizeof(packet), 0);
				if (retval == SOCKET_ERROR) err_display("move key up send()");
			}

			
		}
		
		break;
	}

	case WM_LBUTTONDOWN:
		mousePoint = { LOWORD(lParam),HIWORD(lParam) };

		if (PtInRect(&playButtonRect, mousePoint))
		{
			isReady = true;

			// 윈속 초기화
			if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
				return 1;

			// 소켓 생성
			sock = socket(AF_INET, SOCK_STREAM, 0);
			if (sock == INVALID_SOCKET) err_display("socket()");

			// connect()
			struct sockaddr_in serveraddr;
			memset(&serveraddr, 0, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
			serveraddr.sin_port = htons(PORT);
			retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
			if (retval == SOCKET_ERROR) err_display("connect()");

			hThread = CreateThread(nullptr, 0, NetworkThread,
				reinterpret_cast<LPVOID>(sock), 0, nullptr);
			if (!hThread)
				closesocket(sock);
			else
				CloseHandle(hThread);

			playButtonRect = { 0,0,0,0 };
		}

		if (!isGameStart)
			break;
		else
		{
			CS_CLICK_PACKET packet;
			packet.type = CS_LBUTTONCLICK;
			packet.point = mousePoint;

			char send_buf[BUF_SIZE];
			ZeroMemory(send_buf, BUF_SIZE);
			memcpy(send_buf, &packet, sizeof(packet));

			retval = send(sock, send_buf, sizeof(packet), 0);
			if (retval == SOCKET_ERROR) err_display("move key down send()");
		}
		/*if (PtInRect(&netRect, mousePoint))
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
		}*/

		break;


	case WM_TIMER:
		switch (wParam)
		{
		case 1: {
			hDC = GetDC(hWnd);

			if (hBitmap == NULL)
				hBitmap = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);

			memDC1 = CreateCompatibleDC(hDC);
			memDC2 = CreateCompatibleDC(memDC1);
			oldBit1 = (HBITMAP)SelectObject(memDC1, hBitmap);

			//배경화면
			oldBit2 = (HBITMAP)SelectObject(memDC2, back1);
			StretchBlt(memDC1, 0, 0, rect.right, rect.bottom, memDC2, 0, 0, 256, 192, SRCCOPY);

			/*oldBit2 = (HBITMAP)SelectObject(memDC2, arrow);
			TransparentBlt(memDC1, fish.getRect().left + fish.getWidth() / 3, fish.getRect().top - 30, 30, 50, memDC2, 0, 0, 816, 1083, RGB(255, 255, 255));*/


			if(-1 == fish.is_caught)
				fish.Draw(memDC1, memDC2, normalImage, arrow);
			else
				fish.Draw(memDC1, memDC2, cryImage, arrow);

			for (auto& player : players) {
				if (-1 == player.is_caught) 
					player.Draw(memDC1, memDC2, normalImage, nullptr);
				else 
					player.Draw(memDC1, memDC2, cryImage, nullptr);
			}

			for (auto& player : players)
				player.addMoveCount();
			fish.addMoveCount();

			for (auto& player : players)
			{
				if (player.getMoveCount() > 3)
					player.resetMoveCount();
			}

			if (fish.getMoveCount() > 3)
				fish.resetMoveCount();

			//먹이
			for (auto* f : foods)
			{

				if (f->getFishKinds() == JELLYFISH)
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, jelly);
					TransparentBlt(memDC1, f->getX(), f->getY(), 27, 30, memDC2, 27 * f->getMoveCount(), 0, 27, 30, RGB(255, 1, 1));
				}
				else if (f->getFishKinds() == CRAB)
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, crab);
					TransparentBlt(memDC1, f->getX(), f->getY(), 85, 61, memDC2, 85 * f->getMoveCount(), 0, 85, 61, RGB(255, 1, 1));
				}
				else if (f->getFishKinds() == SQUID)
				{
					oldBit2 = (HBITMAP)SelectObject(memDC2, squid);
					TransparentBlt(memDC1, f->getX(), f->getY(), 42, 72, memDC2, 47 * f->getMoveCount(), 0, 47, 72, RGB(255, 1, 1));
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
				if (netDir == 0)
					TransparentBlt(memDC1, netRect.left, netRect.top, col_x, col_y, memDC2, 1364, 0, 1364, 2438, RGB(255, 1, 1));
				else
					TransparentBlt(memDC1, netRect.left, netRect.top, col_x, col_y, memDC2, 0, 0, 1364, 2438, RGB(255, 1, 1));
				break;

			case 1:
				oldBit2 = (HBITMAP)SelectObject(memDC2, hook);
				TransparentBlt(memDC1, hookRect.left, hookRect.top, 100, 300, memDC2, 0, 0, 536, 1500, RGB(255, 1, 1));
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

				break;
			case 3:
				break;
			}

			/*if (fish.is_caught)
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
			}*/

			if (!isReady && !isGameStart)
			{
				oldBit2 = (HBITMAP)SelectObject(memDC2, playButton);
				TransparentBlt(memDC1, playButtonRect.left, playButtonRect.top, 200, 100, memDC2, 0, 0, 1271, 401, RGB(255, 255, 255));
			}
			else if (isReady)
			{
				//로딩 이미지
				oldBit2 = (HBITMAP)SelectObject(memDC2, loading);
				TransparentBlt(memDC1, loadingRect.left, loadingRect.top, 100, 100, memDC2, 261 * loadingCount, 0, 261, 260, RGB(255, 255, 255));
				++loadingCount;
				if (loadingCount > 7)
					loadingCount = 0;
			}

			SelectObject(memDC2, oldBit2);
			DeleteObject(memDC2);
			SelectObject(memDC1, oldBit1);
			DeleteObject(memDC1);
			InvalidateRgn(hWnd, NULL, false);
			ReleaseDC(hWnd, hDC);
			break;
		}

			//물고기 이동 및 먹이 섭취
		case 3:
			// 다른 플레이어는 살아있을 때만 이동 처리
			for (auto& p : players)
				if (p.GetIsActive())
					if(p.is_caught == -1)
						p.AnimateBySpeed();

			if (fish.is_caught == -1) {
				fish.AnimateBySpeed();

				std::chrono::system_clock::time_point cur = std::chrono::system_clock::now();
				auto interpolation = std::chrono::duration_cast<std::chrono::milliseconds>(cur - fish.last_interpolation).count();

				// 200ms 마다 위치 조정
				if (interpolation > INTERPOLATION_TIME) {
					fish.last_interpolation = cur;

					CS_INTERPOLATION_PACKET packet;
					packet.type = CS_INTERPOLATION;
					packet.x = fish.GetX();
					packet.y = fish.GetY();

					char send_buf[BUF_SIZE];
					ZeroMemory(send_buf, BUF_SIZE);
					memcpy(send_buf, &packet, sizeof(packet));


					retval = send(sock, send_buf, sizeof(packet), 0);
					if (retval == SOCKET_ERROR) err_display("move key up send()");
				}
			}
			else
			{
				/*if (eventNum == 0)
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
			}*/

				if (!fish.is_caught)
				{
					/*for (vector<Food*>::iterator iter = foods.begin(); iter != foods.end(); ++iter)
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
					}*/

					// 클리어
					/*if (fish.getAge() >= 30) {
						KillTimer(hWnd, 1);
						KillTimer(hWnd, 2);
						KillTimer(hWnd, 3);
						KillTimer(hWnd, 4);
						KillTimer(hWnd, 5);
						MessageBox(hWnd, L"개복치가 성장이 완료되었습니다!", L"Congratulations", MB_OK);
						PlaySound(NULL, NULL, NULL);
						PostQuitMessage(0);
					}*/
				}

				break;

				// 이벤트 생성
		case 4:
			//eventNum = rand() % 3;
			//eventClick = 0;
			//eventOut = false;
			//// 화난 얼굴
			//angryCount = 0;
			break;

			// 이벤트 재생
		case 5:
			switch (eventNum)
			{
				/*
					//그물
				case 0:
					if (netDir == 0)
					{
						//netRect = { netRect.left + 10, netRect.top, netRect.right + 10,netRect.bottom };
						netRect = { netRect.left, netRect.top, netRect.right,netRect.bottom };
					}
					else
					{
						//netRect = { netRect.left - 10, netRect.top, netRect.right - 10,netRect.bottom };
						netRect = { netRect.left, netRect.top, netRect.right,netRect.bottom };
					}

					if (!fish.is_caught && !eventOut)
					{
						RECT temp;
						RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
						if (IntersectRect(&temp, &netRect, &fishRect))
						{
							fish.is_caught = true;
						}
					}
					break;

					//낚시 바늘
				case 1:
					if (hookCount < 60)
					{
						hookRect = { hookRect.left,hookRect.top + 5 ,hookRect.right, hookRect.bottom + 5 };
					}
					else if (hookCount >= 60)
					{
						hookRect = { hookRect.left,hookRect.top - 5 ,hookRect.right,hookRect.bottom - 5 };
					}
					++hookCount;

					if (!fish.is_caught && !eventOut)
					{
						RECT temp;
						RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
						RECT hookR = RECT{ hookRect.left,hookRect.top + 240,hookRect.right - 40,hookRect.bottom };
						if (IntersectRect(&temp, &hookR, &fishRect))
						{
							fish.is_caught = true;
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
						if (fish.is_caught)
							fish.setRect(RECT{ fish.getRect().left,fish.getRect().top + 2, fish.getRect().right, fish.getRect().bottom + 2 });
					}
					else if (sharkWave < 24)
					{
						sharkRect = { sharkRect.left, sharkRect.top - 2, sharkRect.right,sharkRect.bottom - 2 };
						if (fish.is_caught)
							fish.setRect(RECT{ fish.getRect().left,fish.getRect().top - 2, fish.getRect().right, fish.getRect().bottom - 2 });
					}
					else
						sharkWave = 0;*/

				/*if (!fish.is_caught && !eventOut)
				{
					RECT temp;
					RECT fishRect = RECT{ fish.getRect().left, fish.getRect().top + (fish.getHeight() / 10 * 2), fish.getRect().right, fish.getRect().bottom - (fish.getHeight() / 10 * 2) };
					if (IntersectRect(&temp, &sharkRect, &fishRect))
					{
						fish.is_caught = true;
					}
				}
				break;*/
			}
			break;

			}
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
		case ID_BACKGROUND_BACK2:
		case ID_BACKGROUND_BACK3:
		case ID_BACKGROUND_BACK4:
			selectBack = 1;
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
		// 소켓 닫기
		closesocket(sock);

		// 윈속 종료
		WSACleanup();
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

void SetFishRect(Fish& fish, const LONG x, const LONG y)
{
	fish.Move(x, y);
}