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

Sprite& DrawableObject::getSprite()
{
	return sprite;
}

void DrawableObject::addAnimation(Animation * a)
{
	a->setOwner(this);
	animations.push(a);
}

void DrawableObject::playAnimation(int _type, int _subtype, bool repeat)
{
	if (!is_active) return;
	currentAnimation = animations.lookObj(_type + _subtype);
	repeatAnimation = repeat;
	currentAnimation->startAnimation();
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