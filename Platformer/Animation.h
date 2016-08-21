#pragma once

class Animation;
class DrawableObject;

#include "RegistratedString.h"
#include <SFML/Graphics.hpp>


using namespace sf;

#define ANIM_TYPE_MULTIPLIER 10000
#define ANIM_CLASS_MULTIPLIER 10000

class Animation
{
	RegistratedString * type;
	RegistratedString * subtype;
	int slides;
	uint timespan; // 0 means static picture, no slide changes
	Texture * texture;
	IntRect * coords;
	Vector2i * delta;
	uint show_time;
	int current_slide;
	DrawableObject * owner;
public:
	Animation(DrawableObject * obj, RegistratedString * _type, RegistratedString * _subtype, int _slides, uint _timespan, Texture * _texture, IntRect * _coords, Vector2i * _delta);
	Animation(const Animation &a);
	~Animation();

	//static int animationType(char * name);
	//static int animationSubType(char * name);
	void setOwner(DrawableObject * o);
	uint UID();
	void startAnimation();
	bool isFinished();
	void Update(uint time_elapsed);
};

