
#include "ofMain.h"
#include "Particle.h"


//  Pure Virtual Function Class - must be subclassed to create new forces.
//
class Force {
protected:
public:
	bool applyOnce = false;
	bool applied = false;
	virtual void updateForce(Particle *) = 0;
};

class ParticleSystem {
public:
	void add(const Particle &);
	void addForce(Force *);
	void remove(int);
	void update();
	void setLifespan(float);
	void reset();
	int removeNear(const ofVec3f & point, float dist);
	void draw();
	vector<Particle> particles;
	vector<Force *> forces;
};



// Some convenient built-in forces
//
class GravityForce: public Force {
	ofVec3f gravity;
public:
	GravityForce(const ofVec3f & gravity);
    void setGravity(const ofVec3f &g);
	void updateForce(Particle *);
    ofVec3f getForce() {return gravity;};
};

class TurbulenceForce : public Force {
	ofVec3f tmin, tmax;
public:
	TurbulenceForce(const ofVec3f & min, const ofVec3f &max);
    void set(const ofVec3f & min, const ofVec3f &max);
	void updateForce(Particle *);
};

class ImpulseRadialForce : public Force {
	float magnitude;
public:
	ImpulseRadialForce(float magnitude); 
	void updateForce(Particle *);
    void setMag(float mag);
    float getMag();
};

class DiskRadialForce : public ImpulseRadialForce {
    float height;
public:
    DiskRadialForce(float magnitute, float height);
    void updateForce(Particle *);
    void setHeight(float h);
};

//class ThrustForce : public ParticleForce { }

