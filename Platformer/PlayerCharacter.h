#pragma once
#include "DrawableObject.h"
class PlayerCharacter : public DrawableObject
{
	Vector2f size;
	Texture * t;
public:
	PlayerCharacter(Vector2f _coords, Vector2f _size, int countOfAnims, Animation ** anims);
	~PlayerCharacter();

	void Update(uint time_elapsed);
	void SendMsg(Msg * msg);
};

