 #include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

// 클라이언트의 정보가 담김
class client {
private:
	short x, y;		// x,y 좌표
	short size;		// 물고기 크기

public:
	bool is_ready;
	int id;			// 클라이언트 구분용 id
	 
public:
	client() {
		x = 0;
		y = 0;
		size = 0;
		id = -1;
		is_ready = false;
	} 
	~client() {}; 

	void SetX(short pos_x) { x = pos_x; }
	void SetY(short pos_y) { y = pos_y; }
	void SetSize(short size) { size = size; }

	short GetX() const { return x; };
	short GetY() const { return y; };
	short GetSize() const { return size; }
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


// 클라이언트 별 쓰레드 생성
DWORD WINAPI RecvThread(LPVOID arg)
{
	int retval;
	SOCKET client_socket = reinterpret_cast<SOCKET>(arg);

	int len{};
	char buf[BUF_SIZE];

	char send_buf[BUF_SIZE];

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

		// 버퍼 처리 
		switch (buf[0]) {
		case CS_PLAYER_MOVE:
			break;

		case CS_LBUTTONCLICK:
			break;

		case CS_LOGIN:
			// 다른 플레이어 정보를 넘김
			for (auto& client : clients) {
				// 접속하지 않았다면 넘김
				if (client.id == -1)
					continue;
				// 대상이 나라면 넘김
				if (client.id == this_id)
					continue;

				SC_ADD_PLAYER_PACKET packet;
				packet.type = SC_ADD_PLAYER;
				packet.id = client.id;

				ZeroMemory(send_buf, sizeof(BUF_SIZE));
				memcpy(send_buf, &packet, sizeof(packet));

				send(client_socket, send_buf, sizeof(packet), 0);
			}

			// 자신의 id를 넘김
			SC_LOGIN_OK_PACKET packet;
			packet.type = SC_LOGIN_OK;

			ZeroMemory(send_buf, sizeof(BUF_SIZE));
			memcpy(send_buf, &packet, sizeof(packet));

			send(client_socket, send_buf, sizeof(packet), 0);

			

			break;

		case CS_PLAYER_READY:
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

			std::cout << ready_count << std::endl;

			// 3명다 준비했다면 GAME_START_PACKET 전송
			if (ready_count == MAX_USER) {
				std::cout << "3명 준비" << std::endl;
				SC_GAME_START_PACKET packet;
				packet.type = SC_GAME_START;

				ZeroMemory(send_buf, sizeof(BUF_SIZE));
				memcpy(send_buf, &packet, sizeof(packet));

				send(client_socket, send_buf, sizeof(packet), 0);
				
			}

			break;

		}

	}

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