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
	int id;			// Ŭ���̾�Ʈ ���п� id

public:
	bool is_ready;

public:
	client() {
		x = 0;
		y = 0;
		size = 0;
		id = 0;
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
std::array<object_info, MAX_OBJECT> objects;	// ������Ʈ ������ ��� �����̳�

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

	while (true) {
		retval = recv(client_socket, buf, BUF_SIZE, MSG_WAITALL);
		if (SOCKET_ERROR == retval) {
			err_display("file size recv()");
			return 0;
		}
		else if (0 == retval) {
			return 0;
		}

		// ���� ó��

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

	while (true) {

		// 3���� ������ accept �� �����ϰ� �ٸ����� �ϵ��� �߰��� ����
		// 3���� ���� ������ �������� �˻��ϴ� ������ �ʿ���

		addrlen = sizeof(addr);
		client_socket = accept(listen_sock, reinterpret_cast<sockaddr*>(&addr), &addrlen);
		if (INVALID_SOCKET == client_socket) {
			err_display("accept()");
			break;
		}

		hThread = CreateThread(nullptr, 0, RecvThread,
			reinterpret_cast<LPVOID>(client_socket), 0, nullptr);
		if (!hThread)
			closesocket(client_socket);
		else
			CloseHandle(hThread);
	}

	closesocket(listen_sock);
	WSACleanup();
}