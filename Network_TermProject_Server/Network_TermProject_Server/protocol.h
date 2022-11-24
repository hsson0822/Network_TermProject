constexpr unsigned short PORT = 9000;
constexpr int BUF_SIZE = 1024;

constexpr int MAX_USER = 3;
constexpr int MAX_OBJECT = 100;

constexpr int SPAWN_WIDTH = 800;
constexpr int SPAWN_HEIGHT = 600;

// packet�� type ����
enum PacketType {
	SC_PLAYER_MOVE,			// ���� -> Ŭ��		�̵�
	CS_PLAYER_MOVE,			// Ŭ�� -> ����		�̵�
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
};

// ������Ʈ type ����
enum ObjectType {
	NET,
	SHARK,
	HOOK,
	CRAB,
	SQUID,
	JELLYFISH
};

// �÷��̾��� �̵�Ű �Է� ����
enum PlayerMove {
	LEFT_DOWN,
	RIGHT_DOWN,
	UP_DOWN,
	DOWN_DOWN,
	LEFT_UP,
	RIGHT_UP,
	UP_UP,
	DOWN_UP
};


// x,y ���� ����ü
struct position {
	short x;
	short y;
};

// ������Ʈ���� ������ ���
struct object_info {
	short x, y;		// ������Ʈ ��ǥ
	char type;		// ������Ʈ ����
};

// ���� -> Ŭ�� ��Ŷ�� id �� Ŭ���̾�Ʈ ���п� id

#pragma pack(push, 1)
struct SC_CREATE_FOOD_PACKET
{
	ObjectType type;
	int id;
	position pos;
};

struct SC_MOVE_PACKET {
	char type;
	int id;
	position pos;
};

struct CS_LOGIN_PACKET {
	char type;
};

struct SC_LOGIN_OK_PACKET {
	char type;
	int id;
};

struct CS_MOVE_PACKET {
	char type;
	char dir;
};

struct CS_CLICK_PACKET {
	char type;
	position pos;
};

struct SC_DEAD_PACKET {
	char type;
	short id;
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

//// �Ʒ��� ����, ��ֹ� ���� ���� ��Ŷ
//// ���� ��Ŀ� ���� �ʿ��� ���� �ְ� �ʿ����� ���� ���� ����
// index �� ���° object �� �߰�/���� �Ǿ����� �ľ�

struct SC_CREATE_OBJCET_PACKET {
	char type;
	int index;
	object_info object;
};

struct SC_ERASE_OBJECT_PACKET {
	char type;
	int index;
};
#pragma pack(pop)