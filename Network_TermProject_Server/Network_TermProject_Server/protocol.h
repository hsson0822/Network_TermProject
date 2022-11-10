constexpr unsigned short PORT = 9000;
constexpr int BUF_SIZE = 1024;

constexpr int MAX_USER = 3;
constexpr int MAX_OBJECT = 100;

// packet의 type 구분
enum PacketType {
	SC_PLAYER_MOVE,			// 서버 -> 클라		이동
	CS_PLAYER_MOVE,			// 클라 -> 서버		이동
	CS_LBUTTONCLICK,		// 클라 -> 서버		왼쪽 마우스버튼 클릭
	SC_PLAYER_DEAD,			// 서버 -> 클라		플레이어 사망
	CS_PLAYER,READY,		// 클라 -> 서버		준비 완료
	SC_GAME_START,			// 서버 -> 클라		게임 시작
	SC_GAME_OVER,			// 서버 -> 클라		게임 종료
	SC_ACCEPT_PACKET,		// 서버 -> 클라		접속 
	SC_ADD_PLAYER,			// 서버 -> 클라		클라이언트 본인 제외 플레이어 추가
	SC_COLLISION,			// 서버 -> 클라		플레이어 충돌
	SC_CREATE_FOOD,			// 서버 -> 클라		먹이 생성
	SC_ERASE_FOOD,			// 서버 -> 클라		먹이 제거
	SC_CREATE_OBSTACLE,		// 서버 -> 클라		장애물 생성
	SC_ERASE_OBSTACLE,		// 서버 -> 클라		장애물 제거
};

// 오브젝트 type 구분
enum ObjectType {
	NET,
	SHARK,
	HOOK,
	CRAB,
	SQUID,
	JELLYFISH
};

// 플레이어의 이동키 입력 구분
enum PlayerMove {
	PLAYER_LEFT_DOWN,
	PLAYER_RIGHT_DOWN,
	PLAYER_UP_DOWN,
	PLAYER_DOWN_DOWN,
	PLAYER_LEFT_UP,
	PLAYER_RIGHT_UP,
	PLAYER_UP_UP,
	PLAYER_DOWN_UP
};


// x,y 묶는 구조체
struct position {
	short x;
	short y;
};

// 오브젝트들의 정보가 담김
struct object_info {
	short x, y;		// 오브젝트 좌표
	char type;		// 오브젝트 종류
};

// 서버 -> 클라 패킷의 id 는 클라이언트 구분용 id

#pragma pack(push, 1)
struct SC_MOVE_PACKET {
	char type;
	int id;
	position pos;
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
	// 게임 시작 시 플레이어들의 정보를 넘길 필요가 있음
	// -> 게임 초기화 작업
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

//// 아래는 먹이, 장애물 생성 제거 패킷
//// 구현 방식에 따라 필요할 수도 있고 필요하지 않을 수도 있음
// index 로 몇번째 object 가 추가/삭제 되었는지 파악

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