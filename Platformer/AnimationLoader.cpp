#include "AnimationLoader.h"
#include "ali.h"
#include "ali_config.h"
#include <list>



AnimationLoader::AnimationLoader(char * xmlfile)
{
	if (xmlfile == NULL)
		strcpy_s(xmlfilename, 256, "animations.data");
	else
		strcpy_s(xmlfilename, 256, xmlfile);
	names_id_counter = 0;
	classnames_id_counter = 0;

	ali_doc_info * doc;
	ali_element_ref doc_root = ali_open(&doc, xmlfilename, ALI_OPTION_INPUT_XML_DECLARATION, NULL);
	if (doc == NULL)
	{
		printf("Error: cannot read %s to read animations.\n", xmlfilename);
		return;
	}

	char tmp_classname[256], tmp_name[256], tmp_texture[256], tmp_type[256], tmp_subtype[256], tmp_slide[256];
	RegistratedString * tmp_regstr;
	uint id;
	int w, h, ts, slides;
	IntRect * coords;
	Vector2i * delta;
	
	ali_element_ref doc_classes = ali_in(doc, doc_root, "^e", 0, "classes");
	while (ali_in(doc, doc_classes, "^oe%s", 0, "class", &tmp_name))
		classnames.push(new RegistratedString(tmp_name, ++classnames_id_counter));

	ali_element_ref doc_animtypes = ali_in(doc, doc_root, "^e", 0, "animtypes");
	while (ali_in(doc, doc_animtypes, "^oe%s", 0, "t", &tmp_name))
		animtypes.push(new RegistratedString(tmp_name, ++types_id_counter));

	ali_element_ref doc_animsubtypes = ali_in(doc, doc_root, "^e", 0, "animsubtypes");
	while (ali_in(doc, doc_animsubtypes, "^oe%s", 0, "st", &tmp_name))
		animsubtypes.push(new RegistratedString(tmp_name, ++subtypes_id_counter));

	ali_element_ref doc_types = ali_in(doc, doc_root, "^e", 0, "types");
	ali_element_ref doc_animatedobjecttype, doc_animation;
	

	while (doc_animatedobjecttype = ali_in(doc, doc_types, "^oe", 0, "animatedobjecttype"))
	{
		ali_in(doc, doc_animatedobjecttype, "^oa%s", 0, "class", &tmp_classname);
		ali_in(doc, doc_animatedobjecttype, "^oa%s", 0, "name", &tmp_name);
		ali_in(doc, doc_animatedobjecttype, "^oa%s", 0, "texture", &tmp_texture);
		ali_in(doc, doc_animatedobjecttype, "^oa%d", 0, "width", &w);
		ali_in(doc, doc_animatedobjecttype, "^oa%d", 0, "height", &h);
		tmp_regstr = new RegistratedString(tmp_name, ++names_id_counter);
		names.push(tmp_regstr);
		Texture * tmp_tex = new Texture();
		textures.push(tmp_tex);
		AnimatedObjectType * at = new AnimatedObjectType(tmp_regstr, RegistratedString::getRSbyName(&classnames, tmp_classname), tmp_tex, tmp_texture, Vector2i(w, h));
		while (doc_animation = ali_in(doc, doc_animatedobjecttype, "^oe", 0, "animation"))
		{
			ali_in(doc, doc_animation, "^oa%d", 0, "timespan",	&ts);
			ali_in(doc, doc_animation, "^oa%s", 0, "type", &tmp_type);
			ali_in(doc, doc_animation, "^oa%s", 0, "subtype", &tmp_subtype);
			ali_in(doc, doc_animation, "^oa%d", 0, "slides", &slides);
			coords = new IntRect[slides];
			delta = new Vector2i[slides];
			int i = 0;
			while (ali_in(doc, doc_animation, "^oe%s", 0, "slide", &tmp_slide) && i < slides)
			{
				sscanf_s(tmp_slide, "%d,%d,%d,%d,%d,%d", &coords[i].left, &coords[i].top, &coords[i].width, &coords[i].height, &delta[i].x, &delta[i].y);
				i++;
			}
			at->addAnimation(new Animation(NULL, RegistratedString::getRSbyName(&animtypes, tmp_type), RegistratedString::getRSbyName(&animsubtypes, tmp_subtype), slides, ts*1000, tmp_tex, coords, delta));
			delete[] coords;
			delete[] delta;
		}
		addType(at);
	}

	ali_close(doc);
}

AnimationLoader::~AnimationLoader()
{
	aotypes.clear();
	classnames.clear();
	names.clear();
	animtypes.clear();
	animsubtypes.clear();
	textures.clear();
}

uint AnimationLoader::addType(AnimatedObjectType * at)
{
	aotypes.push(at);
	return at->UID();
}

void AnimationLoader::loadTextures()
{
	for (AnimatedObjectType * aotype = aotypes.startLoopObj(); aotype != NULL; aotype = aotypes.nextStepObj())
		aotype->loadTexture();
}

AnimatedObjectType * AnimationLoader::getAOType(char * classname, char * name)
{
	return getAOType(RegistratedString::getUIDbyName(&classnames, classname)*ANIM_CLASS_MULTIPLIER + RegistratedString::getUIDbyName(&names, name));
}

AnimatedObjectType * AnimationLoader::getAOType(uint uid)
{
	return aotypes.lookObj(uid);
}

uint AnimationLoader::getAnimationUID(char * type, char * subtype)
{
	return RegistratedString::getUIDbyName(&animtypes, type)*ANIM_TYPE_MULTIPLIER + RegistratedString::getUIDbyName(&animsubtypes, subtype);
}
