 #include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

using namespace std;

// Ŭ���̾�Ʈ�� ������ ���
class client {
private:
	short x, y;		// x,y ��ǥ
	short size;		// ����� ũ��

public:
	bool is_ready;
	int id;			// Ŭ���̾�Ʈ ���п� id
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



// Ŭ���̾�Ʈ �� ������ ����
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

			// ���� ó�� 
			switch (buf[0]) {
			case CS_PLAYER_MOVE: {
				CS_MOVE_PACKET* move_packet = reinterpret_cast<CS_MOVE_PACKET*>(buf);

				client& cl = clients[this_id];

				short x = cl.GetX();
				short y = cl.GetY();

				// ���⿡ ���� x, y ���� �ӵ���ŭ ����
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

				// �浹ó�� �κ� �ʿ�

				// ������ Ŭ���̾�Ʈ ������ �̵��� ��ǥ�� �ֽ�ȭ
				cl.SetX(x);
				cl.SetY(y);

				std::cout << "x : " <<  x << ", y : " << y << ",  speed : " << cl.speed << std::endl;

				SC_MOVE_PACKET packet;
				packet.id = this_id;
				packet.type = SC_PLAYER_MOVE;
				packet.pos.x = x;
				packet.pos.y = y;

				// �� �̵� ������ ��� Ŭ���̾�Ʈ�� ����
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
					// �������� �ʾҴٸ� �ѱ�
					if (client.id == -1)
						continue;
					// ����� ����� �ѱ�
					if (client.id == this_id)
						continue;

					// �ٸ� Ŭ���̾�Ʈ �鿡 �� ������ �ѱ�
					client.send_add_player(this_id);

					// ������ �ٸ� Ŭ���̾�Ʈ ������ �ѱ�
					clients[this_id].send_add_player(client.id);
				}

				// �ڽ��� id�� �ѱ�
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

				// 3��� �غ��ߴٸ� GAME_START_PACKET ����
				if (ready_count == MAX_USER) {
					SC_GAME_START_PACKET packet;
					packet.type = SC_GAME_START;
					
					short x, y;

					for (int i = 0; i < MAX_USER; ++i) {
						x = rand() % SPAWN_WIDTH + 200;
						y = rand() % SPAWN_HEIGHT + 100;
						clients[i].SetX(x);
						clients[i].SetY(y);
						packet.pos[i].x = x;
						packet.pos[i].y = y;

						std::cout << i << " �÷��̾��� ��ǥ : " << x << ", " << y << std::endl;
					}

					for (auto& client : clients)
						client.send_packet(&packet, sizeof(SC_GAME_START_PACKET));

				}

				overload_packet_process(buf, sizeof(CS_READY_PACKET), remain_packet);
				break;
			}

			}
			// ��Ŷ�� ó�� switch

		}
		// ���� ��Ŷ ó��

	}
	// recv ����

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