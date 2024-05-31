#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include <glm/gtx/intersect.hpp>
#include "ParticleSystem.h"
#include "ParticleEmitter.h"


class ofApp : public ofBaseApp{

	public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent2(ofDragInfo dragInfo);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void drawAxis(ofVec3f);
    void initLightingAndMaterials();
    void savePicture();
    void toggleWireframeMode();
    void togglePointsDisplay();
    void toggleSelectTerrain();
    void setCameraTarget();
    bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);
    bool raySelectWithOctree(ofVec3f &pointRet);
    glm::vec3 getMousePointOnPlane(glm::vec3 p , glm::vec3 n);

    ofCamera *theCam = NULL;
    ofEasyCam cam;
    ofCamera fixedCamera;
    ofCamera onBoardCamera;
    ofxAssimpModelLoader terrain, lander;
    string filepath;
    ofLight light;
    Box boundingBox, landerBounds;
    Box testBox;
    vector<Box> colBoxList;
    bool bLanderSelected = false;
    Octree octree;
    TreeNode selectedNode;
    glm::vec3 mouseDownPos, mouseLastPos;
    bool bInDrag = false;

    ofxIntSlider numLevels;
    ofxPanel gui;

    bool bAltKeyDown;
    bool bCtrlKeyDown;
    bool bWireframe;
    bool bDisplayPoints;
    bool bPointSelected;
    bool bHide;
    bool pointSelected = false;
    bool bDisplayLeafNodes = false;
    bool bDisplayOctree = false;
    bool bDisplayBBoxes = false;
    
    bool bLanderLoaded;
    bool bTerrainSelected;

    ofVec3f selectedPoint;
    ofVec3f intersectPoint;

    vector<Box> bboxList;

    const float selectionRange = 4.0;
    
    float start;
    float end;
    ofFile log;
    
// This is for the lander's physics
    ofVec3f forwardHeading();
    ofVec3f leftHeading();
    void thrustForward();
    void thrustBackward();
    void thrustLeft();
    void thrustRight();
    
    ofVec3f velocity;
    ofVec3f acceleration;
    ofVec3f landerForces = gravity;
    
    float angle;
    float angVelocity;
    float angAcceleration_clockwise;
    float angAcceleration_counterClockwise;

    float damping;
    void integrate();
// End lander
    
    GravityForce *gravityForce;
//    GravityForce *thrust;
    ParticleEmitter * emitter;
    TurbulenceForce * turb;
    
    ofVec3f gravity = ofVec3f(0,-1,0);
    ofVec3f turbulance = ofVec3f(0,0,0);
    bool applyForces = true;

    float fuelTime;
    float AGL = 0.0;
    float getAGL();
    
    TreeNode landerHoverNode;
    
    ParticleEmitter * explosionEmitter;
    
    
    // textures
    //
    ofTexture  particleTex;

    // shaders
    //
    ofVbo vbo;
    ofShader shader;
    void loadVbo();
    
    glm::vec3 startingPosition;
    
    ofImage backgroundImage;
    bool bBackgroundLoaded = false;
    
    ofSoundPlayer backgroundSound;
    ofSoundPlayer rocketSound;
    
    enum GameState {
      PRE_GAME,IN_GAME,POST_GAME
    };
    GameState gamestate;
    
    map<int,bool> keymap;
};

