#pragma once
class GameObject;
class DrawableObject;

#include "GameObject.h"
#include "List.h"
#include "Animation.h"
#include "AnimatedObjectType.h"

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

	void initFromAOType(AnimatedObjectType * aot);

	Sprite& getSprite();

	void addAnimation(Animation * a);
	void playAnimation(uint uid, bool repeat = true);
	void playAnimation(char * type, char * subtype, bool repeat = true);
	void updateAnimation(uint time_elapsed);
	void Draw();
};

