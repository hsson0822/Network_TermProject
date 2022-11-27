﻿ #include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

using namespace std;

// 클라이언트의 정보가 담김
class client {
private:
	short x, y;		// x,y 좌표
	short size;		// 물고기 크기

public:
	bool is_ready;
	int id;			// 클라이언트 구분용 id
	int speed;
	SOCKET sock;
	 
public:
	client() {
		x = 0;
		y = 0;
		size = 0;
		id = -1;
		speed = 5;
		is_ready = false;
	} 
	~client() {}; 

	void SetX(short pos_x) { x = pos_x; }
	void SetY(short pos_y) { y = pos_y; }
	void SetSize(short size) { size = size; }

	short GetX() const { return x; };
	short GetY() const { return y; };
	short GetSize() const { return size; }

	void send_packet(void* packet, int size) {
		char send_buf[BUF_SIZE];
		ZeroMemory(send_buf, sizeof(BUF_SIZE));

		memcpy(send_buf, packet, size);

		send(sock, send_buf, size, 0);
	}

	void send_add_player(int id);
};

std::array<client, MAX_USER> clients;			// 클라이언트들의 컨테이너
std::array<object_info, MAX_OBJECT> objects;	// 오브젝트 정 보가 담길 컨테이너

int id = 0;
CRITICAL_SECTION id_cs;
CRITICAL_SECTION cs;

// 오류 검사용 함수
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void client::send_add_player(int id)
{
	SC_ADD_PLAYER_PACKET packet;
	packet.type = SC_ADD_PLAYER;
	packet.id = id;

	send_packet(&packet, sizeof(SC_ADD_PLAYER_PACKET));
}

void overload_packet_process(char* buf, int packet_size, int& remain_packet)
{
	remain_packet -= packet_size;
	if (remain_packet > 0) {
		memcpy(buf, buf + packet_size, BUF_SIZE - packet_size);
	}
}

clock_t start, finish;
double timeNow;

void makeFood()
{

	finish = clock();
	timeNow = (double)(finish - start) / CLOCKS_PER_SEC;
	if (timeNow > 3.0f)
	{

		int foodKinds = rand() % 3;
		short randX = rand() % 1800;
		short randY = rand() % 1000;

		cout << foodKinds << " ���̰� ������ x:" << randX << "  y:" << randY << endl;

		SC_CREATE_FOOD_PACKET packet;

		if (foodKinds == 0)
		{
			//게
			//foods.push_back(new Food(1, randX, randY, 85, 61, 2));
			packet.type = CRAB;
		}
		else if (foodKinds == 1)
		{
			//오징어
			//foods.push_back(new Food(2, randX, randY, 47, 72, 10));
			packet.type = SQUID;
		}
		else
		{
			//해파리
			//foods.push_back(new Food(0, randX, randY, 27, 30, 4));
			packet.type = JELLYFISH;
		}

		start = clock();
		timeNow = 0.0f;

		packet.id = id;
		packet.pos.x = randX;
		packet.pos.y = randY;

		for (auto& client : clients) {
			client.send_packet(&packet, sizeof(packet));
		}

	}

}

DWORD WINAPI CalculateThread(LPVOID arg)
{
	while (true)
	{
		makeFood();

	}

	return 0;
}

// 클라이언트 별 쓰레드 생성
DWORD WINAPI RecvThread(LPVOID arg)
{
	int retval;
	SOCKET client_socket = reinterpret_cast<SOCKET>(arg);

	int len{};
	char buf[BUF_SIZE];
	char send_buf[BUF_SIZE];
	int remain_packet{};

	EnterCriticalSection(&id_cs);
	int this_id = id++;
	LeaveCriticalSection(&id_cs);

	while (true) {
		retval = recv(client_socket, buf, BUF_SIZE, 0);
		if (SOCKET_ERROR == retval) {
			err_display("file size recv()");
			return 0;
		}
		else if (0 == retval) {
			return 0;
		}

		remain_packet = retval;
		while (remain_packet > 0) {

			// 버퍼 처리 
			switch (buf[0]) {
			case CS_PLAYER_MOVE: {
				CS_MOVE_PACKET* move_packet = reinterpret_cast<CS_MOVE_PACKET*>(buf);

				client& cl = clients[this_id];

				short x = cl.GetX();
				short y = cl.GetY();

				// 방향에 따라 x, y 값이 속도만큼 변함
				switch (move_packet->dir) {
				case LEFT_DOWN:
					x -= cl.speed;
					break;
				case RIGHT_DOWN:
					x += cl.speed;
					break;
				case UP_DOWN:
					y -= cl.speed;
					break;
				case DOWN_DOWN:
					y += cl.speed;
					break;
				}

				// 충돌처리 부분 필요

				// 서버의 클라이언트 정보에 이동한 좌표값 최신화
				cl.SetX(x);
				cl.SetY(y);

				std::cout << "x : " <<  x << ", y : " << y << ",  speed : " << cl.speed << std::endl;

				SC_MOVE_PACKET packet;
				packet.id = this_id;
				packet.type = SC_PLAYER_MOVE;
				packet.pos.x = x;
				packet.pos.y = y;

				// 내 이동 정보를 모든 클라이언트에 전송
				for (auto& client : clients) {
					if (client.id == -1)
						continue;
					client.send_packet(&packet, sizeof(packet));
				}

				overload_packet_process(buf, sizeof(CS_MOVE_PACKET), remain_packet);
				break;
			}

			case CS_LBUTTONCLICK: {

				overload_packet_process(buf, sizeof(CS_CLICK_PACKET), remain_packet);
				break;
			}

			case CS_LOGIN: {
				for (auto& client : clients) {
					// 접속하지 않았다면 넘김
					if (client.id == -1)
						continue;
					// 대상이 나라면 넘김
					if (client.id == this_id)
						continue;

					// 다른 클라이언트 들에 내 정보를 넘김
					client.send_add_player(this_id);

					// 나한테 다른 클라이언트 정보를 넘김
					clients[this_id].send_add_player(client.id);
				}

				// 자신의 id를 넘김
				SC_LOGIN_OK_PACKET packet;
				packet.type = SC_LOGIN_OK;
				packet.id = this_id;

				ZeroMemory(send_buf, sizeof(BUF_SIZE));
				memcpy(send_buf, &packet, sizeof(packet));

				send(client_socket, send_buf, sizeof(packet), 0);

				overload_packet_process(buf, sizeof(CS_LOGIN_PACKET), remain_packet);
				break;
			}

			case CS_PLAYER_READY: {
				// 클라이언트로부터 준비완료 패킷을 받으면
				// ready 상태로 변경

				// 임시 동기화
				EnterCriticalSection(&cs);
				clients[this_id].is_ready = true;
				LeaveCriticalSection(&cs);

				int ready_count = 0;
				for (const auto& client : clients)
					if (client.is_ready)
						++ready_count;

				// 3명다 준비했다면 GAME_START_PACKET 전송
				if (ready_count == MAX_USER) {
					SC_GAME_START_PACKET packet;
					packet.type = SC_GAME_START;
					
					// 계산스레드 생성
					HANDLE hThread;
					hThread = CreateThread(nullptr, 0, CalculateThread,
						reinterpret_cast<LPVOID>(client_socket), 0, nullptr);
					
					short x, y;

					for (int i = 0; i < MAX_USER; ++i) {
						x = rand() % SPAWN_WIDTH + 200;
						y = rand() % SPAWN_HEIGHT + 100;
						clients[i].SetX(x);
						clients[i].SetY(y);
						packet.pos[i].x = x;
						packet.pos[i].y = y;

						std::cout << i << " 플레이어의 좌표 : " << x << ", " << y << std::endl;
					}
					for (auto& client : clients)
						client.send_packet(&packet, sizeof(SC_GAME_START_PACKET));

				}

				overload_packet_process(buf, sizeof(CS_READY_PACKET), remain_packet);
				break;
			}

			}
			// 패킷별 처리 switch

		}
		// 남은 패킷 처리

	}
	// recv 종료

	closesocket(client_socket);

	return 0;
} 
 
int main(int argc, char* argv[])
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == listen_sock) {
		err_display("socket()");
		return 1;
	}

	int retval;
	sockaddr_in serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);

	retval = bind(listen_sock, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr));
	if (SOCKET_ERROR == retval) {
		err_display("bind()");
		return 1;
	}
	 
	retval = listen(listen_sock, SOMAXCONN);
	if (SOCKET_ERROR == retval) {
		err_display("listen()");
		return 1;
	}

	SOCKET client_socket;
	sockaddr_in addr;
	int addrlen;
	HANDLE hThread;

	// 동기화 cs, event 초기화
	InitializeCriticalSection(&id_cs);
	InitializeCriticalSection(&cs);
	 
	while (true) {

		// 3명을 받으면 accept 를 종료하고 다른일을 하도록 추가할 예정
		// 3명이 게임 시작을 눌렀는지 검사하는 과정이 필요함

		addrlen = sizeof(addr);
		client_socket = accept(listen_sock, reinterpret_cast<sockaddr*>(&addr), &addrlen);
		if (INVALID_SOCKET == client_socket) {
			err_display("accept()");
			break;
		}

		EnterCriticalSection(&id_cs);
		clients[id].id = id;
		clients[id].sock = client_socket;
		LeaveCriticalSection(&id_cs);

		 
		hThread = CreateThread(nullptr, 0, RecvThread,
			reinterpret_cast<LPVOID>(client_socket), 0, nullptr);
		if (!hThread)
			closesocket(client_socket);
		else
			CloseHandle(hThread);
	}


	DeleteCriticalSection(&id_cs);
	DeleteCriticalSection(&cs);
	

	closesocket(listen_sock);
	WSACleanup();
}