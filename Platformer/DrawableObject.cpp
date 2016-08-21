#include "DrawableObject.h"

DrawableObject::DrawableObject()
{
}

DrawableObject::DrawableObject(Vector2f _coords) : GameObject(_coords)
{
}


DrawableObject::~DrawableObject()
{
	animations.clear();
}

void DrawableObject::initFromAOType(AnimatedObjectType * aot)
{
	if (aot == NULL)
	{
		printf("Houston, we have a problem...\n");
		return;
	}
	Animation ** anims = NULL;
	int animsN = aot->copyAnimations(&anims);
	for (int i = 0; i < animsN; i++)
	{
		anims[i]->setOwner(this);
		animations.push(anims[i]);
	}
	currentAnimation = anims[0];
	delete[] anims;
}

Sprite& DrawableObject::getSprite()
{
	return sprite;
}

void DrawableObject::addAnimation(Animation * a)
{
	a->setOwner(this);
	animations.push(a);
}

void DrawableObject::playAnimation(uint uid, bool repeat)
{
	if (!is_active) return;
	currentAnimation = animations.lookObj(uid);
	repeatAnimation = repeat;
	currentAnimation->startAnimation();
}

void DrawableObject::playAnimation(char * type, char * subtype, bool repeat)
{
	playAnimation(Mgr.getAnimationLoader()->getAnimationUID(type, subtype), repeat);
}

void DrawableObject::updateAnimation(uint time_elapsed)
{
	if (!is_active) return;
	if (!repeatAnimation && currentAnimation->isFinished())
		return;
	currentAnimation->Update(time_elapsed);
}

void DrawableObject::Draw()
{
	if (!is_active) return;
	sprite.setPosition(coords.x, coords.y);
	window.draw(sprite);
}