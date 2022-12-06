#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include <chrono>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

using namespace std;

// 클라이언트의 정보가 담김
class client {
private:
	short x, y;		// x,y 좌표
	short width, height;		// 물고기 크기

public:
	bool is_ready;
	int is_caught;
	bool is_pulled;
	int id;			// 클라이언트 구분용 id
	int speed;
	int score;
	SOCKET sock;


public:
	client() {
		Reset();
	}
	~client() {};

	void Reset() {
		sock = 0;
		x = 0;
		y = 0;
		width = FISH_INIT_WIDTH;
		height = FISH_INIT_HEIGHT;
		id = -1;
		speed = 5;
		score = 0;
		is_ready = false;
		is_caught = -1;
		is_pulled = false;
	}

	void SetX(short pos_x) { x = pos_x; }
	void SetY(short pos_y) { y = pos_y; }
	void SetSize(short si) { width += si; height += si; }
	void Reset() { width = FISH_INIT_WIDTH; height = FISH_INIT_HEIGHT; speed = 5; }

	short GetX() const { return x; };
	short GetY() const { return y; };
	short GetWidth() const { return width; }
	short GetHeight() const { return height; }

	void send_packet(void* packet, int size) {
		char send_buf[BUF_SIZE];
		ZeroMemory(send_buf, sizeof(BUF_SIZE));

		memcpy(send_buf, packet, size);

		send(sock, send_buf, size, 0);
	}

	void send_erase_object(object_info_claculate& oic);
	void send_update_object(object_info_claculate& oic);

	void send_add_player(int id);
};

std::array<client, MAX_USER> clients;			// 클라이언트들의 컨테이너
array<object_info_claculate, MAX_OBJECT> objects_calculate{};
int id_oic = -1;

int id = 0;
CRITICAL_SECTION id_cs;
CRITICAL_SECTION cs;
bool is_game_start = false;

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

void client::send_erase_object(object_info_claculate& oic)
{
	oic.is_active = false;

	SC_ERASE_OBJECT_PACKET packet{};

	if (oic.object_info.type >= CRAB) // CRAB, SQUID, JELLYFISH
		packet.type = SC_ERASE_FOOD;
	else // NET, HOOK, SHARK
		packet.type = SC_ERASE_OBSTACLE;

	packet.object_type = oic.object_info.type;
	packet.index = oic.object_info.id;

	send_packet(&packet, sizeof(SC_ERASE_OBJECT_PACKET));
}

void client::send_update_object(object_info_claculate& oic)
{
	SC_UPDATE_OBJECT_PACKET packet{};

	packet.type = SC_UPDATE_OBSTACLE;

	packet.oi = oic.object_info;

	send_packet(&packet, sizeof(SC_UPDATE_OBJECT_PACKET));
}


void overload_packet_process(char* buf, int packet_size, int& remain_packet)
{
	remain_packet -= packet_size;
	if (remain_packet > 0) {
		memcpy(buf, buf + packet_size, BUF_SIZE - packet_size);
	}
}

chrono::system_clock::time_point foodStart, foodCurrent;
int foodMs{};
void makeFood()
{

	foodCurrent = chrono::system_clock::now();
	foodMs = chrono::duration_cast<chrono::milliseconds>(foodCurrent - foodStart).count();
	// 생성 후 지난 시간이 1500ms 를 넘으면 생성
	if (foodMs > 1500)
	{
		for (const auto& c : clients)
		{
			if (c.id != -1)
				cout << c.id << " 번 플레이어 좌표 x : " << c.GetX() << ", y : " << c.GetY() << endl;
		}

		int foodKinds = rand() % 3 + 3;
		short randX = rand() % 1800;
		short randY = rand() % 1000;

		cout << foodKinds << " 먹이 생성 x:" << randX << "  y:" << randY << endl;

		SC_CREATE_OBJCET_PACKET packet;
		packet.type = SC_CREATE_FOOD;
		short col_x{}, col_y{};

		if (foodKinds == CRAB)
		{
			//게
			packet.object.type = CRAB;
			col_x = CRAB_WIDTH;
			col_y = CRAB_HEIGHT;
		}
		else if (foodKinds == SQUID)
		{
			//오징어
			packet.object.type = SQUID;
			col_x = SQUID_WIDTH;
			col_y = SQUID_HEIGHT;
		}
		else if(foodKinds == JELLYFISH)
		{
			//해파리
			packet.object.type = JELLYFISH;
			col_x = JELLYFISH_WIDTH;
			col_y = JELLYFISH_HEIGHT;
		}

		foodStart = chrono::system_clock::now();

		packet.object.pos.x = randX;
		packet.object.pos.y = randY;


		for (object_info_claculate& oic : objects_calculate)
		{
			if (!oic.is_active)
			{
				id_oic++;
				oic.object_info.id = id_oic;
				oic.is_active = true;
				oic.object_info.type = packet.object.type;
				oic.object_info.pos.x = packet.object.pos.x;
				oic.object_info.pos.y = packet.object.pos.y;
				oic.width = col_x;
				oic.height = col_y;
				break;
			}
			cout << id_oic << endl;
		}
		packet.index = id_oic;

		for (auto& client : clients) {
			if (client.id != -1)
				client.send_packet(&packet, sizeof(SC_CREATE_OBJCET_PACKET));
		}
		cout << "========================" << endl;
	}

}

chrono::system_clock::time_point obstacleStart, obstacleCurrent;
int obstacleMs;
chrono::system_clock::time_point updateStart, updateCurrent;
int updateMs;


void makeObstacle()
{
	obstacleCurrent = chrono::system_clock::now();
	obstacleMs = chrono::duration_cast<chrono::milliseconds>(obstacleCurrent - obstacleStart).count();
	// 생성 후 지난 시간이 30000ms 를 넘으면 생성
	if (obstacleMs > 30000)
	{

		int obstacleKinds = rand() % 3;
		int obstacledir = rand() % 2;
		short randX;
		short randY;
		int obstacleHP = rand() % MAX_LIFE;		// 0 ~ 49 랜덤값

		cout << obstacleKinds << " 장애물 생성" << endl;

		SC_CREATE_OBJCET_PACKET packet;
		packet.type = SC_CREATE_OBSTACLE;
		short col_x{}, col_y{};

		if (obstacleKinds == NET)
		{
			//그물
			packet.object.type = NET;

			if (obstacledir == LEFT)
				randX = WINDOWWIDTH;
			else
				randX = -400;
			randY = 0;
			col_x = NET_WIDTH;
			col_y = NET_HEIGHT;
		}
		else if (obstacleKinds == HOOK)
		{
			//바늘
			packet.object.type = HOOK;

			randX = rand() % 1800;
			randY = 0;
			col_x = HOOK_WIDTH;
			col_y = HOOK_HEIGHT;
		}
		else
		{
			//상어
			packet.object.type = SHARK;

			if (obstacledir == LEFT)
				randX = WINDOWWIDTH;
			else
				randX = -100;
			randY = rand() % 1000;
			col_x = SHARK_WIDTH;
			col_y = SHARK_HEIGHT;
		}

		obstacleStart = chrono::system_clock::now();

		packet.dir = obstacledir;
		packet.object.pos.x = randX;
		packet.object.pos.y = randY;

		for (object_info_claculate& oic : objects_calculate)
		{
			if (!oic.is_active)
			{
				id_oic++;
				oic.object_info.id = id_oic;
				oic.is_active = true;
				oic.object_info.type = packet.object.type;
				oic.object_info.pos.x = packet.object.pos.x;
				oic.object_info.pos.y = packet.object.pos.y;
				oic.width = col_x;
				oic.height = col_y;
				oic.life = obstacleHP;
				oic.dir = packet.dir;
				break;
			}
		}
		packet.index = id_oic;

		for (auto& client : clients) {
			client.send_packet(&packet, sizeof(SC_CREATE_OBJCET_PACKET));
		}
	}
}

void updateObjects()
{
	updateCurrent = chrono::system_clock::now();
	updateMs = chrono::duration_cast<chrono::milliseconds>(updateCurrent - updateStart).count();
	if (updateMs > 17)
	{
		for (object_info_claculate& oic : objects_calculate)
		{
			if (oic.is_active)
			{
				switch (oic.object_info.type)
				{
				case NET:
				{
					if (oic.dir == RIGHT)
						oic.object_info.pos.x += 10;
					else if (oic.dir == LEFT)
						oic.object_info.pos.x -= 10;

					if ((oic.dir == RIGHT && oic.object_info.pos.x >= WINDOWWIDTH) || (oic.dir == LEFT && oic.object_info.pos.x + oic.width <= 0))
					{
						for (client& client : clients)
						{
							if (client.is_caught == oic.object_info.type)
								client.is_caught = -1;
							client.send_erase_object(oic);
						}

					}
					break;
				}
				case SHARK:
				{
					if (oic.dir == RIGHT)
						oic.object_info.pos.x += 7;
					else if (oic.dir == LEFT)
						oic.object_info.pos.x -= 7;

					if ((oic.dir == RIGHT && oic.object_info.pos.x >= WINDOWWIDTH) || (oic.dir == LEFT && oic.object_info.pos.x + oic.width <= 0))
					{
						for (client& client : clients)
						{
							if (client.is_caught == oic.object_info.type)
								client.is_caught = -1;
							client.send_erase_object(oic);
						}
					}
					break;
				}
				case HOOK:
				{
					if (!oic.b_hook)
						oic.object_info.pos.y += 5;
					else if (!oic.b_hook && oic.object_info.pos.y >= 0)
						oic.b_hook = true;
					else if (oic.b_hook)
						oic.object_info.pos.y -= 5;

					if (oic.b_hook && oic.object_info.pos.y < oic.height)
					{
						for (client& client : clients)
						{
							if (client.is_caught == oic.object_info.type)
								client.is_caught = -1;
							client.send_erase_object(oic);
						}
					}
					break;
				}
				default:
				{
					break;
				}
				}
				updateStart = chrono::system_clock::now();

				// switch문 처리 후에도 살아있으면
				if (oic.is_active && oic.object_info.type <= SHARK) // NET, HOOK, SHARK
				{
					for (client& client : clients)
						client.send_update_object(oic);
				}
			}
		}

		for (client& client : clients)
		{
			switch (client.is_caught)
			{
			case NET:
				client.SetX();
				client.SetY();

			}

		}
	}
}

void progress_Collision_pp(RECT tmp, client& cl_1, client& cl_2)
{
	cout << cl_1.id << "번 플레이어와" << cl_2.id << "번 플레이어가 " << tmp.right - tmp.left << " x " << tmp.bottom - tmp.top << " 크기만큼 충돌" << endl;

	if (cl_1.GetWidth() > cl_2.GetWidth())
	{
		// rect가 
		//
		if (cl_2.is_pulled)
		{
			for (client& cl_3 : clients)
			{
				if (cl_3.id != cl_1.id && cl_3.id != cl_2.id)
				{
					if (cl_3.GetWidth() > cl_1.GetWidth())
					{

					}
					else if (cl_3.GetWidth() < cl_1.GetWidth())
					{

					}
					else
					{

					}
				}
			}
		}
	}
	else if (cl_1.GetWidth() < cl_2.GetWidth())
	{
		if (cl_1.is_pulled)
		{
			for (client& cl_3 : clients)
			{
				if (cl_3.id != cl_1.id && cl_3.id != cl_2.id)
				{
					if (cl_3.GetWidth() > cl_2.GetWidth())
					{
					}
					else if (cl_3.GetWidth() < cl_2.GetWidth())
					{

					}
					else
					{

					}
				}
			}
		}
	}
	else
	{

	}
}

void progress_Collision_po(client& client, object_info_claculate& oic)
{
	switch (oic.object_info.type)
	{
	case NET:
	case SHARK:
	case HOOK:
	{
		cout << "충돌 : " << client.id << "번 플레이어, "<< oic.object_info.type << " : " << oic.object_info.id << endl;
		client.is_caught = oic.object_info.type;
		client.score -= OBSTACLE_SCORE;
		client.Reset();
		
		for (auto& cl : clients)
		{
			cl.send_erase_object(oic);
		}

		break;
	}
	case CRAB:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 게 : " << oic.object_info.id << endl;

		client.score += CRAB_SCORE;
		client.SetSize(CRAB_SCORE);
		client.speed *= FISH_INIT_WIDTH / client.GetWidth();
		
		for (auto& cl : clients)
			cl.send_erase_object(oic);

		break;
	}
	case SQUID:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 오징어 : " << oic.object_info.id << endl;
		client.score += SQUID_SCORE;
		client.SetSize(SQUID_SCORE);

		for (auto& cl : clients)
			cl.send_erase_object(oic);

		break;
	}
	case JELLYFISH:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 해파리" << endl;
		client.score += JELLYFISH_SCORE;
		client.SetSize(JELLYFISH_SCORE);

		for (auto& cl : clients)
			cl.send_erase_object(oic);

		break;
	}
	default:
	{
		break;
	}
	}
}

void progress_Collision_mo(object_info_claculate& oic)
{
	if (--oic.life == -1)
	{
		for (auto& cl : clients)
			cl.send_erase_object(oic);
	}

}

void collision()
{
	for (client& cl_1 : clients)
	{
		if (cl_1.id != -1 && !cl_1.is_caught)
		{
			RECT tmp{};
			RECT playerRect_1 = RECT{ cl_1.GetX(), cl_1.GetY(), cl_1.GetX() + cl_1.GetWidth(), cl_1.GetY() + cl_1.GetHeight() };
			for (object_info_claculate& oic : objects_calculate)
			{
				if (oic.is_active)
				{
					RECT objectRect = RECT{ oic.object_info.pos.x, oic.object_info.pos.y, oic.object_info.pos.x + oic.width, oic.object_info.pos.y + oic.height };
					if (IntersectRect(&tmp, &playerRect_1, &objectRect))
						progress_Collision_po(cl_1, oic);
				}
			}
			for (client& cl_2 : clients)
			{
				if (cl_2.id != -1 && cl_2.id != cl_1.id && !cl_2.is_caught)
				{
					RECT playerRect_2 = RECT{ cl_2.GetX(), cl_2.GetY(), cl_2.GetX() + cl_2.GetWidth(), cl_2.GetY() + cl_2.GetHeight() };
					if (IntersectRect(&tmp, &playerRect_1, &playerRect_2))
						progress_Collision_pp(tmp, cl_1, cl_2);
				}
			}
		}
	}
}

DWORD WINAPI CalculateThread(LPVOID arg)
{
	auto start_time = chrono::system_clock::now();
	chrono::system_clock::time_point current_time;
	int duration{};
	foodStart = chrono::system_clock::now();
	obstacleStart = chrono::system_clock::now();
	playerMoveStart = chrono::system_clock::now();

	while (duration < TIME_LIMIT)
	{

		// 플레이어가 한명이라도 있어야만 계산쓰레드가 작동하도록 함
		if (id > 0) {
			makeFood();
			makeObstacle();
			updateObjects();
			collision();
		}
		else
			break;

		// 시작한 시간부터 얼마나 흘렀는지 계산
		// 이 값이 TIME_LIMIT 값보다 크면 시간 초과, 게임 종료
		current_time = chrono::system_clock::now();
		duration = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
	}


	// 종료 패킷 전송
	SC_GAME_OVER_PACKET packet;
	packet.type = SC_GAME_OVER;
	for (int i = 0; i < MAX_USER; ++i)
		packet.scores[i] = clients[i].score;

	for (auto& c : clients) {
		if (-1 == c.id) continue;

		c.send_packet(&packet, sizeof(packet));
	}

	return 0;
}

void disconnect(int c_id)
{
	EnterCriticalSection(&id_cs);
	--id;
	LeaveCriticalSection(&id_cs);
	clients[c_id].Reset();

	SC_LEAVE_PLAYER_PACKET packet;
	packet.type = SC_LEAVE_PLAYER;
	packet.id = c_id;

	for (client& c : clients) {
		if (c_id == c.id) continue;
		if (-1 == c.id) continue;

		c.send_packet(&packet, sizeof(packet));
	}
}

// 클라이언트 별 쓰레드 생성
DWORD WINAPI RecvThread(LPVOID arg)
{
	int retval;
	client_info* c_info = reinterpret_cast<client_info*>(arg);
	SOCKET client_socket = c_info->sock;

	int this_id = c_info->client_id;

	if (id == MAX_USER)
		is_game_start = true;

	int len{};
	char buf[BUF_SIZE];
	char send_buf[BUF_SIZE];
	int remain_packet{};

	while (true) {
		retval = recv(client_socket, buf, BUF_SIZE, 0);
		if (SOCKET_ERROR == retval) {
			err_display("recv()");
			break;
		}
		else if (0 == retval) {
			break;
		}

		remain_packet = retval;
		while (remain_packet > 0) {

			// 버퍼 처리 
			switch (buf[0]) {
			case CS_PLAYER_MOVE: {
				CS_MOVE_PACKET* move_packet = reinterpret_cast<CS_MOVE_PACKET*>(buf);

				client& cl = clients[this_id];

				if (!cl.is_caught)
				{
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


					std::cout << "x : " << x << ", y : " << y << ",  speed : " << cl.speed << std::endl;

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
			}

			case CS_LBUTTONCLICK: {
				CS_CLICK_PACKET* click_packet = reinterpret_cast<CS_CLICK_PACKET*>(buf);

				for (auto& oic : objects_calculate)
				{
					if (oic.is_active && oic.life > 0) // life가 0 이상이면 장애물임
					{
						RECT oicrect = RECT{ oic.object_info.pos.x, oic.object_info.pos.y, oic.object_info.pos.x + oic.width, oic.object_info.pos.y + oic.height };
						if (PtInRect(&oicrect, click_packet->point))
							progress_Collision_mo(oic);
					}
				}
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
					// 계산스레드 생성
					HANDLE hThread = CreateThread(nullptr, 0, CalculateThread,
						reinterpret_cast<LPVOID>(client_socket), 0, nullptr);
				}

				overload_packet_process(buf, sizeof(CS_READY_PACKET), remain_packet);
				break;
			}

			case CS_DISCONNECT: {
				cout << this_id << " 번 플레이어 게임 종료!\n";
				disconnect(this_id);
				closesocket(client_socket);
				return 0;
			}

			}
			// 패킷별 처리 switch

		}
		// 남은 패킷 처리

	}
	// recv 종료

	disconnect(this_id);

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

		client_info c_info;
		EnterCriticalSection(&id_cs);
		clients[id].id = id;
		clients[id].sock = client_socket;
		c_info.client_id = id++;
		LeaveCriticalSection(&id_cs);
		c_info.sock = client_socket;

		hThread = CreateThread(nullptr, 0, RecvThread,
			reinterpret_cast<LPVOID>(&c_info), 0, nullptr);
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