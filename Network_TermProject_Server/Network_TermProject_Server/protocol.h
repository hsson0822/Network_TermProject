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
#define MOVE_BIAS 200				// �̵� ��� �� speed �� ���� ��

#define OBSTACLE_SPAWN_TIME 7000
#define MOVE_COOLTIME 50			// 50ms ���� ��ġ ���
#define INTERPOLATION_TIME 200		// ��ġ ���� 200ms ����

// packet�� type ����
enum PacketType {
	SC_CHANGE_DIRECTION,	// ���� -> Ŭ��		���� ��ȯ
	CS_CHANGE_DIRECTION,	// Ŭ�� -> ����		�̵�
	CS_LBUTTONCLICK,		// Ŭ�� -> ����		���� ���콺��ư Ŭ��
	SC_PLAYER_DEAD,			// ���� -> Ŭ��		�÷��̾� ���
	CS_PLAYER_READY,		// Ŭ�� -> ����		�غ� �Ϸ�
	SC_GAME_START,			// ���� -> Ŭ��		���� ����
	SC_GAME_OVER,			// ���� -> Ŭ��		���� ����
	SC_ACCEPT_PACKET,		// ���� -> Ŭ��		���� 
	SC_ADD_PLAYER,			// ���� -> Ŭ��		Ŭ���̾�Ʈ ���� ���� �÷��̾� �߰�
	SC_COLLISION,			// ���� -> Ŭ��		�÷��̾� �浹
	SC_CREATE_FOOD,			// ���� -> Ŭ��		���� ����
	SC_ERASE_FOOD,			// ���� -> Ŭ��		���� ����
	SC_CREATE_OBSTACLE,		// ���� -> Ŭ��		��ֹ� ����
	SC_ERASE_OBSTACLE,		// ���� -> Ŭ��		��ֹ� ����
	CS_LOGIN,				// Ŭ�� -> ����		�÷��̾� ����
	SC_LOGIN_OK,			// ���� -> Ŭ��		�÷��̾� ���� Ȯ��
	SC_LEAVE_PLAYER,
	CS_DISCONNECT,
	SC_UPDATE_OBSTACLE,
	SC_UPDATE_PLAYER_WH,
	CS_INTERPOLATION,
	SC_INTERPOLATION,
	SC_CAUGHT
};

// ������Ʈ type ����
enum ObjectType {
	NET,
	HOOK,
	SHARK,
	CRAB,
	SQUID,
	JELLYFISH
};

//// �÷��̾��� �̵�Ű �Է� ����
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

// x,y ���� ����ü
struct position {
	short x;
	short y;
};

// ������Ʈ���� ������ ���
struct object_info {
	position pos;		// ������Ʈ ��ǥ
	char type = -1;		// ������Ʈ ����
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
// ���� -> Ŭ�� ��Ŷ�� id �� Ŭ���̾�Ʈ ���п� id

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
	// ���� ���� �� �÷��̾���� ������ �ѱ� �ʿ䰡 ����
	// -> ���� �ʱ�ȭ �۾�
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
	unsigned char num;		// ��� ������Ʈ ��ȭ�� ���޵ɰ���
	object_info* objects;
};

struct SC_ACCEPT_PACKET {		// ������ Ŭ���̾�Ʈ ������ ���� ��Ŷ, �� ����ü�� id�� Ŭ���̾�Ʈ ������ id�� ��
	char type;
	int id;
};

struct SC_ADD_PLAYER_PACKET {	// ������ �������ִ� �ٸ� Ŭ���̾�Ʈ���� ���� ��Ŷ
	char type;
	int id;
};

struct SC_UPDATE_PLAYER_PACKET {	// �÷��̾� �ʺ� ���� ����
	char type;
	int id;
	short w, h;
	int is_caught;
};

//// �Ʒ��� ����, ��ֹ� ���� ���� ��Ŷ
//// ���� ��Ŀ� ���� �ʿ��� ���� �ְ� �ʿ����� ���� ���� ����
// index �� ���° object �� �߰�/���� �Ǿ����� �ľ�

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