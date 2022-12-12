#include <winsock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <array>
#include <chrono>
#include <random>
#include "protocol.h"

#pragma comment(lib,"ws2_32")

using namespace std;

random_device rd;
default_random_engine dre(rd());
uniform_int_distribution<int> random_x(200, PLAYER_WIDTH);		// 플레이어 초기 위치 랜덤
uniform_int_distribution<int> random_y(200, PLAYER_HEIGHT);
uniform_int_distribution<int> random_spawn_x(0, WINDOWWIDTH);	// 오브젝트 랜덤 위치
uniform_int_distribution<int> random_spawn_y(0, WINDOWHEIGHT);	
uniform_int_distribution<int> random_spawn_y_hook(HOOK_HEIGHT, WINDOWHEIGHT - HOOK_HEIGHT);
uniform_int_distribution<int> random_spawn_o_speed(7, 20);
uniform_int_distribution<int> random_life(1, MAX_LIFE);			// 오브젝트 랜덤 체력
uniform_int_distribution<int> random_object(0, 2);			// 오브젝트 랜덤 타입
uniform_int_distribution<int> random_dir(0, 1);

// 클라이언트의 정보가 담김
class client {
private:
	short x, y;		// x,y 좌표
	short width, height;		// 물고기 크기

public:
	bool is_ready;
	int is_caught = -1;
	bool is_pulled;
	int id;			// 클라이언트 구분용 id
	double speed;
	int score;
	SOCKET sock;
	chrono::system_clock::time_point last_move;
	chrono::system_clock::time_point last_interpolation;
	unsigned char dir;
	float duration;


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
		speed = FISH_INIT_SPEED;
		score = 0;
		is_ready = false;
		is_caught = -1;
		is_pulled = false;
		last_move = chrono::system_clock::now();
		last_interpolation = last_move;
		dir = 0;
		duration = MOVE_BIAS;
	}

	void SetX(short pos_x) { x = pos_x; }
	void SetY(short pos_y) { y = pos_y; }
	void SetSizeSpeed(short si) { 
		width += si; 
		height += si; 
		speed *= (double)FISH_INIT_WIDTH / (double)width; 
		if (speed <= FISH_MIN_SPEED)
			speed = FISH_MIN_SPEED;
	}
	void ResetSizeSpeed() { width = FISH_INIT_WIDTH; height = FISH_INIT_HEIGHT; speed = FISH_INIT_SPEED; }

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
	void send_update_object(client& cl);

	void send_add_player(int id);
	
	void ReSpawn();
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

void client::send_update_object(client& cl)
{
	SC_UPDATE_PLAYER_PACKET packet{};

	packet.type = SC_UPDATE_PLAYER_WH;

	packet.w = cl.GetWidth();
	packet.h = cl.GetHeight();
	packet.id = cl.id;
	packet.is_caught = cl.is_caught;
	packet.score = cl.score;

	send_packet(&packet, sizeof(SC_UPDATE_PLAYER_PACKET));
}

void client::ReSpawn()
{
	// 크기, 속도 초기화
	ResetSizeSpeed();
	is_caught = -1;
	score -= OBSTACLE_SCORE;
	if (score < 0)
		score = 0;

	SC_DEAD_PACKET packet;
	packet.type = SC_PLAYER_DEAD;
	packet.id = id;
	packet.score = score;
	short randx = (int)random_x(dre);
	short randy = (int)random_y(dre);

	x = randx;
	y = randy;
	packet.x = randx;
	packet.y = randy;

	for (auto& c : clients)
		if (-1 != c.id)
			c.send_packet(&packet, sizeof(packet));
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
	// 생성 후 지난 시간이 1000ms 를 넘으면 생성
	if (foodMs > FOOD_SPAWN_TIME)
	{
		//for (const auto& c : clients)
		//{
		//	if (c.id != -1)
		//		cout << c.id << " 번 플레이어 좌표 x : " << c.GetX() << ", y : " << c.GetY() << endl;
		//}

		int foodKinds = random_object(dre) + 3;
		short randX = random_spawn_x(dre);
		short randY = random_spawn_y(dre);

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
				cout << id_oic << endl;
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

void makeObstacle()
{
	obstacleCurrent = chrono::system_clock::now();
	obstacleMs = chrono::duration_cast<chrono::milliseconds>(obstacleCurrent - obstacleStart).count();
	// 생성 후 지난 시간이 7초를 넘으면 생성
	if (obstacleMs > OBSTACLE_SPAWN_TIME)
	{

		int obstacleKinds = random_object(dre);
		obstacleKinds = NET;
		int obstacledir = random_dir(dre);
		short randX{};
		short randY{};
		int obstacleHP = random_life(dre);		// hp 랜덤 값
		int y_hook{};
		int speed = random_spawn_o_speed(dre);

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
				randX = -NET_WIDTH;
			col_x = NET_WIDTH;
			uniform_int_distribution<int> random_spawn_h_net(NET_HEIGHT, WINDOWHEIGHT / 2);
			col_y = random_spawn_h_net(dre);
			packet.col_x = col_x;
			packet.col_y = col_y;
			uniform_int_distribution<int> random_spawn_y_net(0, WINDOWHEIGHT - col_y);
			randY = random_spawn_y_net(dre);

		}
		else if (obstacleKinds == HOOK)
		{
			//바늘
			packet.object.type = HOOK;

			randX = random_spawn_x(dre);
			randY = -HOOK_HEIGHT;
			col_x = HOOK_WIDTH;
			col_y = HOOK_HEIGHT;
			packet.col_x = col_x;
			packet.col_y = col_y;
			y_hook = random_spawn_y_hook(dre);
			cout << "y_hook : " << y_hook << endl;
		}
		else
		{
			//상어
			packet.object.type = SHARK;

			if (obstacledir == LEFT)
				randX = WINDOWWIDTH;
			else
				randX = -SHARK_WIDTH;
			randY = random_spawn_y(dre);
			packet.col_x = SHARK_WIDTH;
			packet.col_y = SHARK_HEIGHT;
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
				oic.y_hook = y_hook;
				oic.o_speed = speed;
				break;
			}
		}
		packet.index = id_oic;

		for (auto& client : clients) {
			client.send_packet(&packet, sizeof(SC_CREATE_OBJCET_PACKET));
		}
	}
}

chrono::system_clock::time_point updateStart, updateCurrent;
int updateMs;

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
						oic.object_info.pos.x += oic.o_speed;
					else if (oic.dir == LEFT)
						oic.object_info.pos.x -= oic.o_speed;

					if ((oic.dir == RIGHT && oic.object_info.pos.x >= WINDOWWIDTH) || (oic.dir == LEFT && oic.object_info.pos.x + oic.width <= 0))
					{
						oic.is_active = false;
						for (client& client : clients)
						{
							if (client.id == -1)
								continue;
							if (client.is_caught == oic.object_info.type)
							{
								client.ReSpawn();
								// 리스폰
							}
							client.send_erase_object(oic);
							client.send_update_object(client);
						}

					}
					break;
				}
				case SHARK:
				{
					if (oic.dir == RIGHT)
						oic.object_info.pos.x += oic.o_speed;
					else if (oic.dir == LEFT)
						oic.object_info.pos.x -= oic.o_speed;

					if ((oic.dir == RIGHT && oic.object_info.pos.x >= WINDOWWIDTH) || (oic.dir == LEFT && oic.object_info.pos.x + oic.width <= 0))
					{
						oic.is_active = false;
						for (client& client : clients)
						{
							if (client.id == -1)
								continue;
							if (client.is_caught == oic.object_info.type)
							{
								client.ReSpawn();
								// 리스폰
							}
							client.send_erase_object(oic);
							client.send_update_object(client);
						}
					}
					break;
				}
				case HOOK:
				{
					if (!oic.i_hook && oic.object_info.pos.y < oic.y_hook)
					{
						oic.object_info.pos.y += oic.y_hook / 30;
					}
					else if (oic.i_hook < 60)
					{
						oic.i_hook++;
					}
					else if (oic.object_info.pos.y >= -HOOK_HEIGHT)
					{
						oic.object_info.pos.y -= oic.y_hook / 30;
					}

					if (oic.i_hook >= 60 && oic.object_info.pos.y <= -HOOK_HEIGHT)
					{
						oic.is_active = false;
						for (client& client : clients)
						{
							if (client.id == -1)
								continue;
							else if (client.is_caught == oic.object_info.type)
							{
								client.ReSpawn();
								// 리스폰
							}
							client.send_erase_object(oic);
							client.send_update_object(client);
						}
					}
					break;
				}
				}
				updateStart = chrono::system_clock::now();

				// switch문 처리 후에도 장애물이 아직 살아있으면 잡힌 상태의 플레이어를 장애물에 구속
				if (oic.is_active && oic.object_info.type <= SHARK) // NET, HOOK, SHARK
				{
					for (client& cl : clients)
					{
						if (cl.id == -1)
							continue;
						if (cl.is_caught == oic.object_info.type)
						{
							switch (cl.is_caught)
							{
							case NET:
							case SHARK:
							{
								cl.SetX(oic.object_info.pos.x);
								break;
							}
							case HOOK:
							{
								cl.SetY(oic.object_info.pos.y);
								break;
							}
							}

							SC_CAUGHT_PACKET packet;
							packet.id = cl.id;
							packet.type = SC_CAUGHT;
							packet.x = cl.GetX();
							packet.y = cl.GetY();

							// 내 이동 정보를 모든 클라이언트에 전송
							for (auto& client : clients) {
								if (client.id == -1)
									continue;
								client.send_packet(&packet, sizeof(packet));
							}
							//cl.SetY(oic.object_info.pos.y);
						}

						cl.send_update_object(oic);
						
						
					}
				}
			}
		}
		
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
		//cout << "충돌 : " << client.id << "번 플레이어, "<< oic.object_info.type << " : " << oic.object_info.id << endl;
		client.is_caught = oic.object_info.type;

		for (auto& cl : clients)
		{
			if (cl.id == -1)
				continue;
			cl.send_update_object(client);
		}
		break;
	}
	case CRAB:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 게 : " << oic.object_info.id << endl;

		client.score += CRAB_SCORE;
		client.SetSizeSpeed(CRAB_SCORE);
		oic.is_active = false;

		for (auto& cl : clients)
		{
			if (cl.id == -1)
				continue;
			cl.send_erase_object(oic);
			cl.send_update_object(client);
		}

		break;
	}
	case SQUID:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 오징어 : " << oic.object_info.id << endl;
		client.score += SQUID_SCORE;
		client.SetSizeSpeed(SQUID_SCORE);

		oic.is_active = false;
		for (auto& cl : clients)
		{
			if (cl.id == -1)
				continue;
			cl.send_erase_object(oic);
			cl.send_update_object(client);
		}
			

		break;
	}
	case JELLYFISH:
	{
		cout << "충돌 : " << client.id << "번 플레이어, 해파리" << endl;
		client.score += JELLYFISH_SCORE;
		client.SetSizeSpeed(JELLYFISH_SCORE);

		oic.is_active = false;
		for (auto& cl : clients)
		{
			cl.send_erase_object(oic);
			cl.send_update_object(client);
		}

		break;
	}
	}
}

void progress_Collision_mo(object_info_claculate& oic)
{
	// 클라이언트 쓰레드마다 오브젝트의 체력에 접근할 수 있으므로 lock 필요
	oic.life_lock.lock();
	if (--oic.life == -1)
	{
		oic.is_active = false;
		oic.life_lock.unlock();

		for (auto& cl : clients) {
			if (-1 == cl.id) continue;

			// 잡혀있었다면 잡힌 플레이어를 놔줘야 함
			// 사라진 object 타입과 is_caught 가 같다면 잡혔던 플레이어
			if (oic.object_info.type == cl.is_caught) {
				cl.is_caught = -1;

				for (auto& c : clients) {
					if (-1 == cl.id) continue;
					
					c.send_update_object(cl);
				}
			}

			cl.send_erase_object(oic);
		}
	}
	else
		oic.life_lock.unlock();

}

void collision()
{	
	short bias{};
	for (client& cl_1 : clients)
	{
		if (cl_1.id != -1 && cl_1.is_caught == -1)
		{
			RECT tmp{};
			RECT playerRect_1 = RECT{ cl_1.GetX() + bias, cl_1.GetY() + bias, cl_1.GetX() + cl_1.GetWidth() - bias, cl_1.GetY() + cl_1.GetHeight() - bias };
			for (object_info_claculate& oic : objects_calculate)
			{
				if (oic.is_active)
				{
					RECT objectRect = RECT{ oic.object_info.pos.x, oic.object_info.pos.y, oic.object_info.pos.x + oic.width, oic.object_info.pos.y + oic.height };
					if (IntersectRect(&tmp, &playerRect_1, &objectRect))
						progress_Collision_po(cl_1, oic);
				}
			}
		}
	}
}

void MovePlayer()
{
	std::chrono::system_clock::time_point cur;

	// 이동 쿨타임 마다 위치 갱신
	for (auto& client : clients) {
		//printf("플레이어 : %d, is_caught : %d\n", client.id, client.is_caught);
		if (-1 == client.id) continue;
		if (-1 != client.is_caught) continue;

		// 현재 시간 - 마지막으로 움직인 시간
		cur = std::chrono::system_clock::now();
		auto exec = std::chrono::duration_cast<std::chrono::milliseconds>(cur - client.last_move).count();

		// 쿨타임 50ms 보다 차이가 크면 이동
		if (exec > MOVE_COOLTIME) {
			client.last_move = cur;

			unsigned char dir = client.dir;

			// 이동하지 않는다면 아래 이동 처리 건너뜀
			if (dir == 0)
				continue;

			short x = client.GetX();
			short y = client.GetY();
			
			float dist = client.speed / client.duration * exec;

			if ((dir & MOVE_LEFT) == MOVE_LEFT)
				x -= dist;
			if ((dir & MOVE_RIGHT) == MOVE_RIGHT)
				x += dist;
			if ((dir & MOVE_UP) == MOVE_UP)
				y -= dist;
			if ((dir & MOVE_DOWN) == MOVE_DOWN)
				y += dist;

			client.SetX(x);
			client.SetY(y);

		}

		//// 500ms 마다 위치 조정	-> 클라이언트에서 서버로 조정하는 방식으로 됨, 원래는 서버에서 클라이언트로 보정하는 것이 맞을것
		//auto interpolation = std::chrono::duration_cast<std::chrono::milliseconds>(cur - client.last_interpolation).count();
		//if (interpolation > INTERPOLATION_TIME) {
		//	client.last_interpolation = cur;


		//	short x = client.GetX();
		//	short y = client.GetY();

		//	SC_INTERPOLATION_PACKET packet;
		//	packet.id = client.id;
		//	packet.type = SC_INTERPOLATION;
		//	packet.x = x;
		//	packet.y = y;

		//	for (auto& c : clients) {
		//		if (-1 == c.id) continue;
		//		c.send_packet(&packet, sizeof(packet));
		//	}


		//}
	}

}

DWORD WINAPI CalculateThread(LPVOID arg)
{
	auto start_time = chrono::system_clock::now();
	chrono::system_clock::time_point current_time;
	int duration{};
	foodStart = chrono::system_clock::now();
	obstacleStart = chrono::system_clock::now();
	updateStart = chrono::system_clock::now();

	while (duration < TIME_LIMIT)
	{
		// 플레이어가 한명이라도 있어야만 계산쓰레드가 작동하도록 함
		if (id > 0) {
			MovePlayer();
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

	if (id > 0) {
		// 종료 패킷 전송
		SC_GAME_OVER_PACKET packet;
		packet.type = SC_GAME_OVER;
		for (int i = 0; i < MAX_USER; ++i)
			packet.scores[i] = clients[i].score;

		for (auto& c : clients) {
			if (-1 == c.id) continue;

			c.send_packet(&packet, sizeof(packet));
		}
	}

	id_oic = -1;
	for (auto& oic : objects_calculate)
		oic.is_active = false;
	

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
			case CS_CHANGE_DIRECTION: {

				client& c = clients[this_id];

				CS_CHANGE_DIRECTION_PACKET* move_packet = reinterpret_cast<CS_CHANGE_DIRECTION_PACKET*>(buf);
				c.dir = move_packet->dir;

				SC_CHANGE_DIRECTION_PACKET packet;
				packet.type = SC_CHANGE_DIRECTION;
				packet.id = this_id;
				packet.dir = move_packet->dir;
				packet.speed = c.speed;

				for (auto& client : clients) {
					if (-1 == client.id) continue;

					client.send_packet(&packet, sizeof(packet));
				}



				overload_packet_process(buf, sizeof(SC_CHANGE_DIRECTION_PACKET), remain_packet);
				break;

			}

			case CS_INTERPOLATION: {
				CS_INTERPOLATION_PACKET* packet = reinterpret_cast<CS_INTERPOLATION_PACKET*>(buf);
				short x = packet->x;
				short y = packet->y;

				clients[this_id].SetX(x);
				clients[this_id].SetY(y);

				// 서버에서 보간작업 이후 다른 클라이언트에도 보내줘야 함
				// 자기 자신은 이미 처리했기 때문에 보내줄 필요 없음
				SC_INTERPOLATION_PACKET p;
				p.type = SC_INTERPOLATION;
				p.id = this_id;
				p.x = x;
				p.y = y;
				
				for (auto& client : clients) {
					if (client.id == -1) continue;
					if (client.id == this_id) continue;
					
					client.send_packet(&p, sizeof(p));
				}


				overload_packet_process(buf, sizeof(CS_INTERPOLATION_PACKET), remain_packet);
				break;
			}

			case CS_LBUTTONCLICK: {
				CS_CLICK_PACKET* click_packet = reinterpret_cast<CS_CLICK_PACKET*>(buf);
				for (auto& oic : objects_calculate)
				{
					if (oic.is_active && oic.life >= 0) // life가 0 이상이면 장애물임
					{
						RECT oicrect = RECT{ oic.object_info.pos.x, oic.object_info.pos.y, oic.object_info.pos.x + oic.width, oic.object_info.pos.y + oic.height };
						if (PtInRect(&oicrect, click_packet->point)) {
							progress_Collision_mo(oic);
						}
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
						x = random_x(dre);
						y = random_y(dre);
						clients[i].SetX(x);
						clients[i].SetY(y);
						packet.pos[i].x = x;
						packet.pos[i].y = y;
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