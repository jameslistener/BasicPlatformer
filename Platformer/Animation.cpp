#include "Animation.h"
#include "DrawableObject.h"

Animation::Animation(DrawableObject * obj, RegistratedString * _type, RegistratedString * _subtype, int _slides, uint _timespan, Texture * _texture, IntRect * _coords, Vector2i * _delta)
{
	type = _type;
	subtype = _subtype;
	slides = _slides;
	timespan = _timespan;
	texture = _texture;
	show_time = 0;
	owner = obj;
	coords = new IntRect[slides];
	delta = new Vector2i[slides];
	for (int i = 0; i < slides; i++)
	{
		coords[i] = _coords[i];
		delta[i] = _delta[i];
	}
	current_slide = 0;
}

Animation::Animation(const Animation & a)
{
	type = a.type;
	subtype = a.subtype;
	slides = a.slides;
	timespan = a.timespan;
	texture = a.texture;
	show_time = 0;
	owner = NULL;
	coords = new IntRect[slides];
	delta = new Vector2i[slides];
	for (int i = 0; i < slides; i++)
	{
		coords[i] = a.coords[i];
		delta[i] = a.delta[i];
	}
}

Animation::~Animation()
{
	delete[] coords;
	delete[] delta;
}
/*
int Animation::animationType(char * name)
{
	if		(strcmp(name, "IDLE") == 0) return A_T_IDLE;
	else if (strcmp(name, "IDLE") == 0) return A_T_IDLE;
	return 0;
}*/

void Animation::setOwner(DrawableObject * o)
{
	owner = o;
}

uint Animation::UID()
{
	return type->UID()*ANIM_TYPE_MULTIPLIER + subtype->UID();
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
