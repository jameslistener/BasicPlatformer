#pragma once
#include "DrawableObject.h"
class PlayerCharacter : public DrawableObject
{
	Vector2f size;
public:
	PlayerCharacter(Vector2f _coords, Vector2f _size);
	~PlayerCharacter();

	void Update(uint time_elapsed);
	void SendMsg(Msg * msg);
};

