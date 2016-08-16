#pragma once
class DrawableObject;

#include "GameObject.h"
#include "List.h"
#include "Animation.h"
using namespace sf;

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

