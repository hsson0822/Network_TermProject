#pragma once
enum
{
	NET,
	SHARK,
	HOOK,
	CRAB,
	SQUID,
	JELLYFISH
};
enum {
	SC_READY,
	SC_GAMESTART,
	SC_PLAYER_POS,
	SC_CREATE_FOOD,
	SC_ERASE_FOOD,
	SC_CREATE_OBSTACLE,
	SC_ERASE_OBSTACLE,
	SC_GAME_OVER,
	CS_READY,
	CS_PLAYER_LEFT_DOWN,
	CS_PLAYER_RIGHT_DOWN,
	CS_PLAYER_UP_DOWN,
	CS_PLAYER_DOWN_DOWN,
	CS_PLAYER_LEFT_UP,
	CS_PLAYER_RIGHT_UP,
	CS_PLAYER_UP_UP,
	CS_PLAYER_DOWN_UP,
	CS_PLAYER_COLLISION,
	CS_PLAYER_DEAD,
	CS_CLICK_OBSTACLE
};
