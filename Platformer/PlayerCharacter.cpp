#include "PlayerCharacter.h"


PlayerCharacter::PlayerCharacter(Vector2f _coords, Vector2f _size, int countOfAnims, Animation ** anims) : DrawableObject(_coords)
{
	size = _size;
	for (int i = countOfAnims; i > 0; i--)
		animations.push(anims[i]);
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
