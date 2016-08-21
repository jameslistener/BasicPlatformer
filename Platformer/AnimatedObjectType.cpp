#include "AnimatedObjectType.h"


AnimatedObjectType::AnimatedObjectType()
{
}


AnimatedObjectType::AnimatedObjectType(RegistratedString * name, RegistratedString * classname, Texture * texture, char * texturefile, Vector2i & size)
{
	this->name = name;
	this->classname = classname;
	this->texture = texture;
	strcpy_s(this->texturefile, 255, texturefile);
	this->size = Vector2i(size);
	uid = classname->UID()*ANIM_CLASS_MULTIPLIER + name->UID();
}

AnimatedObjectType::~AnimatedObjectType()
{
	delete texture;
	anims.clear();
}

void AnimatedObjectType::addAnimation(Animation * a)
{
	anims.push(a);
}

uint AnimatedObjectType::UID()
{
	return uid;
}

void AnimatedObjectType::getName(char * str)
{
	name->getStr(str);
}

void AnimatedObjectType::getClassName(char * str)
{
	classname->getStr(str);
}

Texture * AnimatedObjectType::getTexture()
{
	return texture;
}

Vector2i AnimatedObjectType::getSize()
{
	return size;
}

int AnimatedObjectType::copyAnimations(Animation *** animations)
{
	this;
	int n = anims.getSize();
	*animations = new Animation *[n];

	int i = 0;
	for (Animation * a = anims.startLoopObj(); a != NULL; a = anims.nextStepObj())
		(*animations)[i++] = new Animation(*a);

	return n;
}

void AnimatedObjectType::loadTexture()
{
	texture->loadFromFile(texturefile);
}
