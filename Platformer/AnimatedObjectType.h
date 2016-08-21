#pragma once

class AnimatedObjectType;
class RegistratedString;

#include "Animation.h"
#include "AnimationLoader.h"
#include "List.h"
//#include "GameObject.h"
#include <SFML/Graphics.hpp>

using namespace sf;

class AnimatedObjectType
{
private:
	RegistratedString * classname;
	RegistratedString * name;
	char texturefile[256];
	Texture * texture;
	Vector2i size;
	List<Animation> anims;

	uint uid;

	AnimatedObjectType(); //so no one can create empty object
public:

	AnimatedObjectType(RegistratedString * name, RegistratedString * classname, Texture * texture, char * texturefile, Vector2i &size);
	~AnimatedObjectType();

	void addAnimation(Animation * a);
	
	// getters
	uint UID();
	void getName(char * str);
	void getClassName(char * str);
	Texture * getTexture();
	Vector2i getSize();
	
	int copyAnimations(Animation *** animations);
	void loadTexture();

};

