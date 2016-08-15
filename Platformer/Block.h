#pragma once
class Block;
#include "DrawableObject.h"

class Block : public DrawableObject
{
	Vector2f size; // x is width, y is height
public:
	Block();
	Block(Vector2f _coords, Vector2f _size, Texture * t, IntRect &ac);
	~Block();

	void Update(uint time_elapsed);
	void SendMsg(Msg * msg);

};

