#pragma once

class Animation;

#include "DrawableObject.h"

#define A_T_NONE 0
#define A_T_IDLE 10000
#define A_T_WALK 20000
#define A_T_JUMP 30000
#define A_T_FALL 40000
#define A_T_HIT 50000
#define A_T_GET_HIT 60000
#define A_T_DO_SMTHING 70000
#define A_T_SPECIAL 80000
#define A_T_DEATH 90000
#define A_T_MAGIC 100000

#define A_S_NONE 0
#define A_S_LEFT 100
#define A_S_RIGHT 200
#define A_S_FRONT 300
#define A_S_BACK 400


class Animation
{
	int type;
	int subtype;
	int slides;
	uint timespan; // 0 means static picture, no slide changes
	Texture * texture;
	IntRect * coords;
	Vector2i * delta;
	uint show_time;
	int current_slide;
	DrawableObject * owner;
public:
	Animation(DrawableObject * obj, int _type, int _subtype, int _slides, uint _timespan, Texture * _texture, IntRect * _coords, Vector2i * _delta);
	Animation(const Animation &a);
	~Animation();

	void setOwner(DrawableObject * o);
	uint UID();
	void startAnimation();
	bool isFinished();
	void Update(uint time_elapsed);
};

