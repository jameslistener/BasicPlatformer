#include "AnimatedType.h"



AnimatedType::AnimatedType()
{
}


AnimatedType::AnimatedType(char * name, char * classname, char * texturefile, Vector2i & size)
{
	strcpy(this->name, name);
	strcpy(this->classname, classname);
	strcpy(this->texturefile, texturefile);
	this->size = Vector2i(size);
	texture = new Texture();
	texture->loadFromFile(texturefile);
}

AnimatedType::~AnimatedType()
{
	delete texture;
	anims.clear;
}

void AnimatedType::addAnimation(Animation * a)
{
	anims.push(a);
}

uint AnimatedType::UID()
{
	return uid;
}

char * AnimatedType::getName()
{
	return name;
}

char * AnimatedType::getClassName()
{
	return classname;
}

Texture * AnimatedType::getTexture()
{
	return texture;
}

Vector2i AnimatedType::getSize()
{
	return size;
}

int AnimatedType::copyAnimations(Animation ** animations)
{
	int n = anims.getSize();
	animations = new Animation *[n];

	for (Animation * a = anims.startLoopObj(), int i = 0; a != NULL; a = anims.nextStepObj())
		animations[i++] = new Animation(*a);

	return n;
}
