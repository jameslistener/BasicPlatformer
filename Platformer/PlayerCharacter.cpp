#include "PlayerCharacter.h"


PlayerCharacter::PlayerCharacter(Vector2f _coords, Vector2f _size) : DrawableObject(_coords)
{
	size = _size;
	initFromAOType(Mgr.getAnimationLoader()->getAOType("Character", "Jack"));
}

PlayerCharacter::~PlayerCharacter()
{
}

void PlayerCharacter::Update(uint time_elapsed)
{
	currentAnimation->Update(time_elapsed);
}

void PlayerCharacter::SendMsg(Msg * msg)
{

}
