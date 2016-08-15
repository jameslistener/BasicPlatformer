#include "Block.h"



Block::Block() : DrawableObject()
{
}

Block::Block(Vector2f _coords, Vector2f _size, Texture * t, IntRect & ac) : DrawableObject(_coords)
{
	size = _size;
	IntRect * acs = new IntRect(ac);
	Animation * a = new Animation(this, A_T_IDLE, 1, 1, 0, t, acs);
	delete acs;
	addAnimation(a);
	a->startAnimation();
}




Block::~Block()
{
}

void Block::Update(uint time_elapsed)
{
	;
}

void Block::SendMsg(Msg * msg)
{
	switch (msg->type)
	{
	case 1:
		break;
	default:
		break;
	}
	delete msg;
}
