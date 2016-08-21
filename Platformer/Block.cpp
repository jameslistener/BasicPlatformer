#include "Block.h"



Block::Block() : DrawableObject()
{
}

Block::Block(Vector2f _coords, Vector2f _size) : DrawableObject(_coords)
{
	size = _size;
	initFromAOType(Mgr.getAnimationLoader()->getAOType("StaticBlock", "Ground"));
	playAnimation("IDLE", "FIRST");
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
