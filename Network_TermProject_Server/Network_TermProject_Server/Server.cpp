 #include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

// Ŭ���̾�Ʈ�� ������ ���
class client {
private:
	short x, y;		// x,y ��ǥ
	short size;		// ����� ũ��

public:
	bool is_ready;
	int id;			// Ŭ���̾�Ʈ ���п� id
	 
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

std::array<client, MAX_USER> clients;			// Ŭ���̾�Ʈ���� �����̳�
std::array<object_info, MAX_OBJECT> objects;	// ������Ʈ �� ���� ��� �����̳�

int id = 0;
CRITICAL_SECTION id_cs;
CRITICAL_SECTION cs;

// ���� �˻�� �Լ�
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


// Ŭ���̾�Ʈ �� ������ ����
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

		// ���� ó�� 
		switch (buf[0]) {
		case CS_PLAYER_MOVE:
			break;

		case CS_LBUTTONCLICK:
			break;

		case CS_LOGIN:
			// �ٸ� �÷��̾� ������ �ѱ�
			for (auto& client : clients) {
				// �������� �ʾҴٸ� �ѱ�
				if (client.id == -1)
					continue;
				// ����� ����� �ѱ�
				if (client.id == this_id)
					continue;

				SC_ADD_PLAYER_PACKET packet;
				packet.type = SC_ADD_PLAYER;
				packet.id = client.id;

				ZeroMemory(send_buf, sizeof(BUF_SIZE));
				memcpy(send_buf, &packet, sizeof(packet));

				send(client_socket, send_buf, sizeof(packet), 0);
			}

			// �ڽ��� id�� �ѱ�
			SC_LOGIN_OK_PACKET packet;
			packet.type = SC_LOGIN_OK;

			ZeroMemory(send_buf, sizeof(BUF_SIZE));
			memcpy(send_buf, &packet, sizeof(packet));

			send(client_socket, send_buf, sizeof(packet), 0);

			

			break;

		case CS_PLAYER_READY:
			// Ŭ���̾�Ʈ�κ��� �غ�Ϸ� ��Ŷ�� ������
			// ready ���·� ����
			
			// �ӽ� ����ȭ
			EnterCriticalSection(&cs);
			clients[this_id].is_ready = true;
			LeaveCriticalSection(&cs);

			int ready_count = 0;
			for (const auto& client : clients)
				if (client.is_ready)
					++ready_count;

			std::cout << ready_count << std::endl;

			// 3��� �غ��ߴٸ� GAME_START_PACKET ����
			if (ready_count == MAX_USER) {
				std::cout << "3�� �غ�" << std::endl;
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

	// ����ȭ cs, event �ʱ�ȭ
	InitializeCriticalSection(&id_cs);
	InitializeCriticalSection(&cs);
	 
	while (true) {

		// 3���� ������ accept �� �����ϰ� �ٸ����� �ϵ��� �߰��� ����
		// 3���� ���� ������ �������� �˻��ϴ� ������ �ʿ���

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