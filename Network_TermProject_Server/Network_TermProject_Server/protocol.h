#pragma once
#include <mutex>

constexpr unsigned short PORT = 9000;
constexpr int BUF_SIZE = 1024;

constexpr int TIME_LIMIT = 180;
constexpr int MAX_USER = 3;
constexpr int MAX_OBJECT = 100;

constexpr int PLAYER_WIDTH = 1000;
constexpr int PLAYER_HEIGHT = 600;

#define WINDOWWIDTH 1800
#define WINDOWHEIGHT 900
#define OBSTACLE_SCORE 10
#define MAX_LIFE 1

#define NET_WIDTH 200
#define NET_HEIGHT 400
#define HOOK_WIDTH 100
#define HOOK_HEIGHT 300
#define SHARK_WIDTH 200
#define SHARK_HEIGHT 100

#define CRAB_WIDTH 85
#define CRAB_HEIGHT 61
#define SQUID_WIDTH 42
#define SQUID_HEIGHT 72
#define JELLYFISH_WIDTH 27
#define JELLYFISH_HEIGHT 30

#define FISH_INIT_WIDTH 120
#define FISH_INIT_HEIGHT 140
#define FISH_INIT_SPEED 40
#define FISH_MIN_SPEED 30

#define MOVE_LEFT  0b0001
#define MOVE_RIGHT 0b0010
#define MOVE_UP	   0b0100
#define MOVE_DOWN  0b1000
#define MOVE_BIAS 200				// 이동 계산 시 speed 에 나눌 값

#define OBSTACLE_SPAWN_TIME 7000
#define MOVE_COOLTIME 50			// 50ms 마다 위치 계산
#define INTERPOLATION_TIME 200		// 위치 조정 200ms 마다

// packet의 type 구분
enum PacketType {
	SC_CHANGE_DIRECTION,	// 서버 -> 클라		방향 전환
	CS_CHANGE_DIRECTION,	// 클라 -> 서버		이동
	CS_LBUTTONCLICK,		// 클라 -> 서버		왼쪽 마우스버튼 클릭
	SC_PLAYER_DEAD,			// 서버 -> 클라		플레이어 사망
	CS_PLAYER_READY,		// 클라 -> 서버		준비 완료
	SC_GAME_START,			// 서버 -> 클라		게임 시작
	SC_GAME_OVER,			// 서버 -> 클라		게임 종료
	SC_ACCEPT_PACKET,		// 서버 -> 클라		접속 
	SC_ADD_PLAYER,			// 서버 -> 클라		클라이언트 본인 제외 플레이어 추가
	SC_COLLISION,			// 서버 -> 클라		플레이어 충돌
	SC_CREATE_FOOD,			// 서버 -> 클라		먹이 생성
	SC_ERASE_FOOD,			// 서버 -> 클라		먹이 제거
	SC_CREATE_OBSTACLE,		// 서버 -> 클라		장애물 생성
	SC_ERASE_OBSTACLE,		// 서버 -> 클라		장애물 제거
	CS_LOGIN,				// 클라 -> 서버		플레이어 접속
	SC_LOGIN_OK,			// 서버 -> 클라		플레이어 접속 확인
	SC_LEAVE_PLAYER,
	CS_DISCONNECT,
	SC_UPDATE_OBSTACLE,
	SC_UPDATE_PLAYER_WH,
	CS_INTERPOLATION,
	SC_INTERPOLATION,
	SC_CAUGHT
};

// 오브젝트 type 구분
enum ObjectType {
	NET,
	HOOK,
	SHARK,
	CRAB,
	SQUID,
	JELLYFISH
};

//// 플레이어의 이동키 입력 구분
//enum PlayerMove {
//	MOVE_LEFT,
//	MOVE_RIGHT,
//	MOVE_UP,
//	MOVE_DOWN
//};

enum FoodScore {
	CRAB_SCORE = 1,
	SQUID_SCORE,
	JELLYFISH_SCORE
};

enum ObstacleDirection {
	RIGHT,
	LEFT
};

struct client_info {
	SOCKET sock;
	int client_id;
};

// x,y 묶는 구조체
struct position {
	short x;
	short y;
};

// 오브젝트들의 정보가 담김
struct object_info {
	position pos;		// 오브젝트 좌표
	char type = -1;		// 오브젝트 종류
	int id = -1;
};

struct object_info_claculate {
	object_info object_info;
	bool is_active = false;
	short width, height;
	int i_hook = 0;
	int y_hook;
	int life = -1;
	int dir = -1;
	std::mutex life_lock;
};
// 서버 -> 클라 패킷의 id 는 클라이언트 구분용 id

#pragma pack(push, 1)
struct SC_CREATE_FOOD_PACKET
{
	char type;
	object_info object;
	int id;
	position pos;
};

struct SC_CHANGE_DIRECTION_PACKET {
	char type;
	unsigned char dir;
	int id;
	int speed;
};

struct CS_LOGIN_PACKET {
	char type;
};

struct SC_LOGIN_OK_PACKET {
	char type;
	int id;
};

struct CS_CHANGE_DIRECTION_PACKET {
	char type;
	unsigned char dir;
};

struct CS_CLICK_PACKET {
	char type;
	POINT point;
};

struct SC_DEAD_PACKET {
	char type;
	short id;
	short x, y;
	int score;
};

struct CS_READY_PACKET {
	char type;
	short id;
};

struct SC_GAME_START_PACKET {
	char type;
	// 게임 시작 시 플레이어들의 정보를 넘길 필요가 있음
	// -> 게임 초기화 작업
	position pos[3];
};

struct SC_COLLISION_PACKET {
	unsigned char size;
	char type;
	int id;
	int* scores;
};

struct SC_GAME_OVER_PACKET {
	char type;
	int scores[3];
};

struct SC_OBJECT_PACKET {
	char type;
	unsigned char num;		// 몇개의 오브젝트 변화가 전달될건지
	object_info* objects;
};

struct SC_ACCEPT_PACKET {		// 접속한 클라이언트 본인이 받을 패킷, 이 구조체의 id가 클라이언트 본인의 id가 됨
	char type;
	int id;
};

struct SC_ADD_PLAYER_PACKET {	// 기존에 접속해있던 다른 클라이언트들이 받을 패킷
	char type;
	int id;
};

struct SC_UPDATE_PLAYER_PACKET {	// 플레이어 너비 높이 변경
	char type;
	int id;
	short w, h;
	int is_caught;
};

//// 아래는 먹이, 장애물 생성 제거 패킷
//// 구현 방식에 따라 필요할 수도 있고 필요하지 않을 수도 있음
// index 로 몇번째 object 가 추가/삭제 되었는지 파악

struct SC_CREATE_OBJCET_PACKET {
	char type;
	int index;
	object_info object;
	unsigned char dir;
};

struct SC_ERASE_OBJECT_PACKET {
	char type;
	int index;
	char object_type = -1;
};

struct SC_UPDATE_OBJECT_PACKET {
	char type;
	object_info oi;
};

struct SC_LEAVE_PLAYER_PACKET {
	char type;
	int id;
};

struct CS_DISCONNECT_PACKET {
	char type;
};

struct CS_INTERPOLATION_PACKET {
	char type;
	short x, y;
};

struct SC_INTERPOLATION_PACKET {
	char type;
	int id;
	short x, y;
};

struct SC_CAUGHT_PACKET {
	char type;
	int id;
	short x, y;
};
#pragma pack(pop)