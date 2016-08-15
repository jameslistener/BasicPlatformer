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

Animation::Animation(DrawableObject * obj, int _type, int _subtype, int _slides, uint _timespan, Texture * _texture, IntRect * _coords, Vector2f * _delta)
{
	type = _type;
	subtype = _subtype;
	slides = _slides;
	timespan = _timespan;
	texture = _texture;
	show_time = 0;
	owner = obj;
	coords = new IntRect[slides];
	delta = new Vector2f[slides];
	for (int i = 0; i < slides; i++)
	{
		coords[i] = _coords[i];
		delta[i] = _delta[i];
	}
	current_slide = 0;
}

Animation::~Animation()
{
	delete[] coords;
}

uint Animation::UID()
{
	return type+subtype;
}

void Animation::startAnimation()
{
	show_time = 0;
	current_slide = 0;
	Sprite &s = owner->getSprite();
	if (s.getTexture() != texture)
		s.setTexture(*texture);
	s.setTextureRect(IntRect(coords[0]));
}

bool Animation::isFinished()
{
	if (timespan == 0) return true;
	return (show_time >= timespan*slides);
}

void Animation::Update(uint time_elapsed)
{
	if (!(owner->isActive()) || timespan == 0) return;
	show_time += time_elapsed;
	current_slide = ((int)(show_time / timespan)) % slides;
	Sprite &s = owner->getSprite();
	if (s.getTexture() != texture)
		s.setTexture(*texture);
	s.setTextureRect(IntRect(coords[current_slide]));
}
