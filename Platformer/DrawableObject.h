#pragma once
class DrawableObject;
class Animation;

#include "GameObject.h"
#include "List.h"
using namespace sf;

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
	Vector2f * delta;
	uint show_time;
	int current_slide;
	DrawableObject * owner;
public:
	Animation(DrawableObject * obj, int _type, int _subtype, int _slides, uint _timespan, Texture * _texture, IntRect * _coords, Vector2f * _delta);
	~Animation();
	uint UID();
	void startAnimation();
	bool isFinished();
	void Update(uint time_elapsed);
};

class DrawableObject : public GameObject
{
protected:
	Sprite sprite;
	List<Animation> animations;
	Animation * currentAnimation;
	bool repeatAnimation;

public:
	DrawableObject();
	DrawableObject(Vector2f _coords);
	~DrawableObject();
	Sprite& getSprite();

	void addAnimation(Animation * a);
	void playAnimation(int _type, int _subtype, bool repeat = true);
	void updateAnimation(uint time_elapsed);
	void Draw();
};

