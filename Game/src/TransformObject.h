#pragma once
#include "ofMain.h"


//  Base class for any object that needs a transform.
//
class TransformObject {
public:
	TransformObject();
	ofVec3f position, scale;
	float	rotation;
	bool	bSelected;
	void setPosition(const ofVec3f &);
};
