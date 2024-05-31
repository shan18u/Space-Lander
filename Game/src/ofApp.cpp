
#include "ofApp.h"
#include "Util.h"


//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup(){
	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bLanderLoaded = false;
	bTerrainSelected = true;
//	ofSetWindowShape(1024, 768);
	cam.setDistance(10);
	cam.setNearClip(.1);
	cam.setFov(65.5);   // approx equivalent to 28mm in 35mm format
	ofSetVerticalSync(true);
	cam.disableMouseInput();
	ofEnableSmoothing();
	ofEnableDepthTest();

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();
    
//    filepath = "mars-low-5x-v2.obj";
 //   filepath = "moon-houdini.obj";
//    filepath = "terrain.obj";
      filepath = "land12.obj";
 //   filepath = "teraintest.obj";
    if(!terrain.loadModel("geo/" + filepath)) {
        cout << "Couldn't load terrain" << endl;
        ofExit();
    }
	terrain.setScaleNormalization(false);
    
    
    bBackgroundLoaded = backgroundImage.load("images/starfield-plain.jpg");
    if(!bBackgroundLoaded) {
        cout << "Couldn't load starfield image" << endl;
        ofExit();
    }
    
    // texture loading
    //
    ofDisableArbTex();     // disable rectangular textures

    // load textures
    //
    if (!ofLoadImage(particleTex, "images/dot.png")) {
        cout << "Particle Texture File: images/dot.png not found" << endl;
        ofExit();
    }

    // load the shader
    //
    #ifdef TARGET_OPENGLES
        shader.load("shaders_gles/shader");
    #else
        shader.load("shaders/shader");
    #endif
    
    if (!backgroundSound.load("sounds/Apollo11Onboard.mp3") || !rocketSound.load("sounds/rocket.mp3")) {
        cout << "Sound file not found" << endl;
        ofExit();
    }
    
	// create sliders for testing
	//
	gui.setup();
	gui.add(numLevels.setup("Number of Octree Levels", 1, 1, 10));
	bHide = false;

	//  Create Octree for testing.
	//
	
    start=ofGetElapsedTimeMillis();
	octree.create(terrain.getMesh(0), 20);
    end = ofGetElapsedTimeMillis();

    
    filepath = filepath.replace(filepath.length()-3, filepath.length(), "txt");
    ofFile log("logs/" + filepath);
    if (!log.exists()) {
        log.create();
    }
    log.open("logs/" + filepath, ofFile::Append);
    log << "Creation time: " <<  end-start << " ms" <<  endl;
    log << "Number of Verts: " << terrain.getMesh(0).getNumVertices() << endl;
    log.close();
    
	testBox = Box(Vector3(3, 3, 0), Vector3(5, 5, 2));
    
    // lander stuff
    velocity = ofVec3f(0,0,0);
    acceleration = ofVec3f(0,0,0);
    damping=0.9999;
    
    angle = 0.0;
    angVelocity = 0.0;
    angAcceleration_clockwise = 0.0;
    angAcceleration_counterClockwise = 0.0;
    // end lander stuff
    
    gravityForce = new GravityForce(gravity);
//    gravityForce->applyOnce=false;
//    thrust = new GravityForce(ofVec3f(0,0,0));
    turb = new TurbulenceForce(ofVec3f(-5,-5,-5),ofVec3f(5,5,5));
    
    
    emitter = new ParticleEmitter();
    emitter->setParticleRadius(3);
    emitter->setRate(75);
    emitter->visible=false;
    emitter->setPosition(ofVec3f(0,0,0));
    emitter->sys->addForce(turb);
    emitter->sys->addForce(gravityForce);
    
    explosionEmitter = new ParticleEmitter();
    explosionEmitter->setEmitterType(RadialEmitter);
    explosionEmitter->setOneShot(true);
    explosionEmitter->setGroupSize(1000);
    explosionEmitter->setLifespan(10);
    explosionEmitter->setParticleRadius(10);
    explosionEmitter->setVelocity(ofVec3f(0,500,0));
    explosionEmitter->sys->addForce(gravityForce);

//    emitter->sys->addForce(gravityForce);
//    emitter->setVelocity(gravity);
    
    fixedCamera = ofCamera();
    fixedCamera.setPosition(0, 25, 0);
    fixedCamera.setNearClip(.1);
    fixedCamera.setFov(65.5);   // approx equivalent to 28mm in 35mm format
    fixedCamera.lookAt(glm::vec3(0, 0, 0));
    
    onBoardCamera = ofCamera();
    onBoardCamera.setOrientation(ofVec3f(-90,180,0));
    onBoardCamera.setPosition(0, 25, 0);
    onBoardCamera.setNearClip(.1);
    onBoardCamera.setFov(65.5);   // approx equivalent to 28mm in 35mm format
    
    
    theCam=&fixedCamera;
    
    gamestate = PRE_GAME;
    applyForces = false;
    acceleration = gravity;
    
    startingPosition = lander.getPosition();
    fuelTime = 120;
}
 
//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {
    
    switch (gamestate) {
        case PRE_GAME:
            break;
        case IN_GAME:
            
            // kepmaps to update the thrusters on the lander IF there is enough fuel remaining
            if (fuelTime>0) {
                
                // Up Thrust
                if (keymap['w']) {
                    fuelTime-=1/ofGetFrameRate();
                    landerForces += ofVec3f(0,2,0);
                    if(!emitter->started) {
                        emitter->start();
                    }
                } else {
                    if(emitter->started) {
                        emitter->stop();
                    }
                }
                // Down Thrust
                if (keymap['s']) {
                    fuelTime-=1/ofGetFrameRate();
                    landerForces += ofVec3f(0,-.1,0);
                }
                if (keymap['a']) {
                    fuelTime-=1/ofGetFrameRate();
                    angAcceleration_clockwise=3.0;
                } else {
                    angAcceleration_clockwise=0;
                }
                if (keymap['d']) {
                    fuelTime-=1/ofGetFrameRate();
                    angAcceleration_counterClockwise=-3.0;
                } else {
                    angAcceleration_counterClockwise=0;
                }
                if (keymap[OF_KEY_UP]) {
                    fuelTime-=1/ofGetFrameRate();
                    thrustForward();
                }
                if (keymap[OF_KEY_DOWN]) {
                    fuelTime-=1/ofGetFrameRate();
                    thrustBackward();
                }
                if (keymap[OF_KEY_LEFT]) {
                    fuelTime-=1/ofGetFrameRate();
                    thrustLeft();
                }
                if (keymap[OF_KEY_RIGHT]) {
                    fuelTime-=1/ofGetFrameRate();
                    thrustRight();
                }
            } else if (emitter->started) {
                    emitter->stop();
            }
            
            if (fuelTime<0) {
                fuelTime=0.0;
            }
            
            if (glm::length(glm::vec3(landerForces.x,landerForces.y,landerForces.z))>0.0) {
                if (!rocketSound.isPlaying()) {
                    rocketSound.play();
                }
            } else if (rocketSound.isPlaying()){
                rocketSound.stop();
            }
            
            if (colBoxList.size()>=10) {
                gamestate = POST_GAME;
                if(emitter->started) {
                    emitter->stop();
                }
            }
            break;
        case POST_GAME:
            break;
        default:
            break;
    }
    
    // collision detection and resolution
    if (colBoxList.size()>=10) {

        velocity = .45*reflectVector(velocity, octree.mesh.getNormal(landerHoverNode.points[0]));
        if (glm::length(glm::vec3(velocity.x,velocity.y,velocity.z))>2.0) {
            velocity*=10;
            explosionEmitter->setPosition(lander.getPosition());
            explosionEmitter->start();
        }
    }
    
    if (bLanderLoaded) {
        float FR = ofGetFrameRate();
           if (FR!=0) {
               integrate();
           }
        ofVec3f min = lander.getSceneMin() + lander.getPosition();
        ofVec3f max = lander.getSceneMax() + lander.getPosition();

        landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

        colBoxList.clear();
        octree.intersect(landerBounds, octree.root, colBoxList);
        
        onBoardCamera.setPosition(glm::vec3(lander.getPosition().x,lander.getPosition().y,lander.getPosition().z));
        onBoardCamera.setOrientation(ofVec3f(-90,angle - 180,0));
        fixedCamera.lookAt(glm::vec3(lander.getPosition().x,lander.getPosition().y,lander.getPosition().z));
        
        emitter->setPosition(lander.getPosition()+ofVec3f(0,.22,0));
    }
    
    emitter->update();
    explosionEmitter->update();
}
//--------------------------------------------------------------
void ofApp::draw() {

//	ofBackground(ofColor::black);
    
    
    loadVbo();
    
    // draw a background image such as a starfield
    if (bBackgroundLoaded) {
        ofPushMatrix();
        ofDisableDepthTest();
        ofSetColor(3*50, 3*50, 3*50);
        ofScale(2, 2);
        backgroundImage.draw(-200, -100);
        ofEnableDepthTest();
        ofPopMatrix();
    }

	glDepthMask(false);
	if (!bHide) gui.draw();
	glDepthMask(true);

	theCam->begin();
	ofPushMatrix();
//    if (theCam==&onBoardCamera) {
//        ofRotate(90, 0, -1, 0);
//    }
    
	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		terrain.drawWireframe();
		if (bLanderLoaded) {
			lander.drawWireframe();
			if (!bTerrainSelected) drawAxis(lander.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();              // shaded mode
		terrain.drawFaces();
		ofMesh mesh;
		if (bLanderLoaded) {
			lander.drawFaces();
			if (!bTerrainSelected) drawAxis(lander.getPosition());
			if (bDisplayBBoxes) {
				ofNoFill();
				ofSetColor(ofColor::white);
				for (int i = 0; i < lander.getNumMeshes(); i++) {
					ofPushMatrix();
					ofMultMatrix(lander.getModelMatrix());
					ofRotate(-90, 1, 0, 0);
					Octree::drawBox(bboxList[i]);
					ofPopMatrix();
				}
			}

			if (bLanderSelected) {

				ofVec3f min = lander.getSceneMin() + lander.getPosition();
				ofVec3f max = lander.getSceneMax() + lander.getPosition();

				Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
				ofSetColor(ofColor::white);
				Octree::drawBox(bounds);

				// draw colliding boxes
				//
                
				ofSetColor(ofColor::yellow);
                if (colBoxList.size()>=10) {
                    ofSetColor(ofColor::green);
                }
				for (int i = 0; i < colBoxList.size(); i++) {
					Octree::drawBox(colBoxList[i]);
				}
			}
		}
	}
	if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));



	if (bDisplayPoints) {                // display points as an option    
		glPointSize(3);
		ofSetColor(ofColor::green);
		terrain.drawVertices();
	}

	// highlight selected point (draw sphere around selected point)
	//
	if (bPointSelected) {
		ofSetColor(ofColor::blue);
		ofDrawSphere(selectedPoint, .1);
	}

	// recursively draw octree
	//
	ofDisableLighting();
	int level = 0;
	//	ofNoFill();

	if (bDisplayLeafNodes) {
		octree.drawLeafNodes(octree.root);
		cout << "num leaf: " << octree.numLeaf << endl;
    }
	else if (bDisplayOctree) {
		ofNoFill();
		ofSetColor(ofColor::white);
		octree.draw(numLevels, 0);
	}

	// if point selected, draw a sphere
	//
	if (pointSelected) {
		ofVec3f p = octree.mesh.getVertex(selectedNode.points[0]);
		ofVec3f d = p - cam.getPosition();
		ofSetColor(ofColor::lightGreen);
		ofDrawSphere(p, .02 * d.length());
	}
    
	ofPopMatrix();
	theCam->end();
    
    glDepthMask(GL_FALSE);
    ofSetColor(255, 100, 90);
    // this makes everything look glowy :)
    //
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofEnablePointSprites();
    shader.begin();
    theCam->begin();
    emitter->draw();
    explosionEmitter->draw();
    particleTex.bind();
    vbo.draw(GL_POINTS, 0, (int)(emitter->sys->particles.size() + explosionEmitter->sys->particles.size()));
    particleTex.unbind();
    theCam->end();
    shader.end();
    
    ofDisablePointSprites();
    ofDisableBlendMode();
    ofEnableAlphaBlending();
    glDepthMask(GL_TRUE);
    
    string str;
    str += "Frame Rate: " + std::to_string(ofGetFrameRate());
    ofSetColor(ofColor::white);
    ofDrawBitmapString(str, ofGetWindowWidth() -375, 15);
    
    string str1;
    str1 += "Acceleration: (" + std::to_string(acceleration.x) +"," + std::to_string(acceleration.y)+ "," +std::to_string(acceleration.z) + ")";
    ofSetColor(ofColor::white);
    ofDrawBitmapString(str1, ofGetWindowWidth() -375, 30);
    
    string str2;
    str2 += "Velocity: (" + std::to_string(velocity.x) +"," + std::to_string(velocity.y)+ "," +std::to_string(velocity.z) + ")";
    ofSetColor(ofColor::white);
    ofDrawBitmapString(str2, ofGetWindowWidth() -375, 45);
    
    string str3;
    str3 += "ALT/AGL: " + std::to_string(AGL);
    ofSetColor(ofColor::white);
    ofDrawBitmapString(str3, ofGetWindowWidth() -375, 60);
    
    string str4;
    str4 += "Fuel Time: " + std::to_string(fuelTime);
    ofSetColor(ofColor::white);
    ofDrawBitmapString(str4, ofGetWindowWidth() -375, 75);
}


// 
// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));
	

	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}

void ofApp::keyPressed(int key) {
    keymap[key] = true;
	switch (key) {
    case ' ':
            // a feature to move the spacecraft manually and start the game from there
            if (gamestate==PRE_GAME) {
                applyForces=true;
                gamestate=IN_GAME;
                backgroundSound.play();
            } else if(gamestate==POST_GAME) {
                lander.setPosition(startingPosition.x, startingPosition.y, startingPosition.z);
                acceleration=gravity;
                applyForces=false;
                gamestate=PRE_GAME;
                backgroundSound.stop();
            }
        break;
    case 'm':
        toggleWireframeMode();
        break;
	case 'B':
	case 'b':
		bDisplayBBoxes = !bDisplayBBoxes;
		break;
	case 'C':
	case 'c':
		if (cam.getMouseInputEnabled()) cam.disableMouseInput();
		else cam.enableMouseInput();
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
    case 'g':
        savePicture();
        break;
	case 'H':
	case 'h':
		break;
	case 'L':
	case 'l':
		bDisplayLeafNodes = !bDisplayLeafNodes;
		break;
	case 'O':
	case 'o':
		bDisplayOctree = !bDisplayOctree;
		break;
	case 'r':
		cam.reset();
		break;
	case 't':
		setCameraTarget();
		break;
	case 'u':
		break;
	case 'v':
		togglePointsDisplay();
		break;
	case 'V':
		break;
	case OF_KEY_ALT:
		cam.enableMouseInput();
		bAltKeyDown = true;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = true;
		break;
	case OF_KEY_SHIFT:
		break;
	case OF_KEY_DEL:
		break;
    case OF_KEY_F1:
        theCam = &cam;
        break;
    case OF_KEY_F2:
        theCam = &onBoardCamera;
        break;
    case OF_KEY_F3:
        theCam = &fixedCamera;
        break;
	default:
		break;
	}
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

void ofApp::keyReleased(int key) {
    keymap[key] = false;
	switch (key) {
	
	case OF_KEY_ALT:
		cam.disableMouseInput();
		bAltKeyDown = false;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = false;
		break;
	case OF_KEY_SHIFT:
		break;
	default:
		break;

	}
}



//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	// if moving camera, don't allow mouse interaction
//
	if (cam.getMouseInputEnabled()) return;

	// if rover is loaded, test for selection
	//
	if (bLanderLoaded) {
		glm::vec3 origin = cam.getPosition();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = lander.getSceneMin() + lander.getPosition();
		ofVec3f max = lander.getSceneMax() + lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			bLanderSelected = true;
			mouseDownPos = getMousePointOnPlane(lander.getPosition(), cam.getZAxis());
			mouseLastPos = mouseDownPos;
			bInDrag = true;
		}
		else {
			bLanderSelected = false;
		}
	}
	else {
        start = ofGetElapsedTimeMillis();
		raySelectWithOctree(selectedPoint);
        end = ofGetElapsedTimeMillis();
        
        log.open("logs/" + filepath, ofFile::Append);
        log << "Search time: " <<  end-start << " ms" <<  endl;

        log.close();
        
	}
}

bool ofApp::raySelectWithOctree(ofVec3f &pointRet) {
	ofVec3f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));

	pointSelected = octree.intersect(ray, octree.root, selectedNode);

	if (pointSelected) {
		pointRet = octree.mesh.getVertex(selectedNode.points[0]);
    }

// my attempt at point search, the idea is that a 30-60-90 triangle is formed between the mouse ray and the camera origin to vertex vector, and we know the length of that side/vector is x*sqrt(3), so we should be able to calculate the length of the hypotonous through the x,2x,x*sqrt(3) relationship. Then we just want to loop and see which vertex in the selected node has minimum distance to origin + direction * hypotonous. it does not work perfectly though in some parts of the terrain/when zoomed in
    else {
        float minD = INFINITY;
        int minIndex = -1;
        for (int i = 0; i<selectedNode.points.size(); i++) {
            glm::vec3 v = octree.mesh.getVertex(selectedNode.points[i]);
            glm::vec3 B = v-glm::vec3(ray.origin.x(),ray.origin.y(),ray.origin.z());
            float hypotonous = 2*glm::length(B)/sqrt(3);
            Vector3 q = ray.origin + ray.direction*hypotonous;
            float tmp = glm::length(glm::vec3(q.x(),q.y(),q.z()) -v);
            if (tmp<minD) {
                minD = tmp;
                minIndex = i;
            }
        }
        
        if (minIndex!=-1) {
            pointRet=octree.mesh.getVertex(selectedNode.points[minIndex]);
            pointSelected=true;
        }
    }
	return pointSelected;
}




//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	if (bInDrag) {

		glm::vec3 landerPos = lander.getPosition();

		glm::vec3 mousePos = getMousePointOnPlane(landerPos, cam.getZAxis());
		glm::vec3 delta = mousePos - mouseLastPos;
	
		landerPos += delta;
		lander.setPosition(landerPos.x, landerPos.y, landerPos.z);
        
        startingPosition = landerPos;
		mouseLastPos = mousePos;

//		ofVec3f min = lander.getSceneMin() + lander.getPosition();
//		ofVec3f max = lander.getSceneMax() + lander.getPosition();
//
//		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
//
//		colBoxList.clear();
//		octree.intersect(bounds, octree.root, colBoxList);
	

//		if (bounds.overlap(testBox)) {
//			cout << "overlap" << endl;
//		}
//		else {
//			cout << "OK" << endl;
//		}


	}
	else {
        start = ofGetElapsedTimeMillis();
        raySelectWithOctree(selectedPoint);
        end = ofGetElapsedTimeMillis();
        
        log.open("logs/" + filepath, ofFile::Append);
        log << "Search time: " <<  end-start << " ms" <<  endl;

        log.close();
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {
    theCam->lookAt(selectedPoint);
}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}



//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
} 

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent2(ofDragInfo dragInfo) {

	ofVec3f point;
	mouseIntersectPlane(ofVec3f(0, 0, 0), cam.getZAxis(), point);
	if (lander.loadModel(dragInfo.files[0])) {
		lander.setScaleNormalization(false);
//		lander.setScale(.1, .1, .1);
		lander.setPosition(point.x, point.y, point.z);
        startingPosition=lander.getPosition();

		bLanderLoaded = true;
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		cout << "Mesh Count: " << lander.getMeshCount() << endl;
	}
	else cout << "Error: Can't load model" << dragInfo.files[0] << endl;
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	ofVec2f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (lander.loadModel(dragInfo.files[0])) {
		bLanderLoaded = true;
		lander.setScaleNormalization(false);
		lander.setPosition(0, 0, 0);
		cout << "number of meshes: " << lander.getNumMeshes() << endl;
		bboxList.clear();
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		//		lander.setRotation(1, 180, 1, 0, 0);

				// We want to drag and drop a 3D object in space so that the model appears 
				// under the mouse pointer where you drop it !
				//
				// Our strategy: intersect a plane parallel to the camera plane where the mouse drops the model
				// once we find the point of intersection, we can position the lander/lander
				// at that location.
				//

				// Setup our rays
				//
		glm::vec3 origin = cam.getPosition();
		glm::vec3 camAxis = cam.getZAxis();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
		if (hit) {
			// find the point of intersection on the plane using the distance 
			// We use the parameteric line or vector representation of a line to compute
			//
			// p' = p + s * dir;
			//
			glm::vec3 intersectPoint = origin + distance * mouseDir;

			// Now position the lander's origin at that intersection point
			//
			glm::vec3 min = lander.getSceneMin();
			glm::vec3 max = lander.getSceneMax();
			float offset = (max.y - min.y) / 2.0;
			lander.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

			// set up bounding box for lander while we are at it
			//
			landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		}
        startingPosition = lander.getPosition();
	}


}

//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = cam.getPosition();
	glm::vec3 camAxis = cam.getZAxis();
	glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}

ofVec3f ofApp::forwardHeading() {
    return glm::normalize(glm::rotate(glm::mat4(1.0), glm::radians(angle), glm::vec3(0, 1, 0)) * glm::vec4(glm::vec3(0, 0, 1), 1));
}

ofVec3f ofApp::leftHeading() {
    return glm::normalize(glm::rotate(glm::mat4(1.0), glm::radians(angle), glm::vec3(1, 0, 0)) * glm::vec4(glm::vec3(1, 0, 0), 1));
}

void ofApp::thrustForward() {
    landerForces+=forwardHeading()*.1;
}

void ofApp::thrustBackward() {
    landerForces+=-forwardHeading()*.1;
}

void ofApp::thrustLeft() {
    landerForces+=leftHeading()*.1;
}

void ofApp::thrustRight() {
    landerForces=-leftHeading()*.1;
}

void ofApp::integrate() {
    // (1) update position from velocity and time interval
    
    float FR = ofGetFrameRate();
//    cout << FR << endl;
    if (FR!=0) {
        float dt = 1.0/FR;
        
        if (!applyForces) {
            acceleration=gravity;
            velocity=ofVec3f(0,0,0);
        } else {
            
            ofVec3f position = lander.getPosition()+velocity*dt;
            lander.setPosition(position.x,position.y,position.z);
            
            // (2) update velocity (based on acceleration
            velocity +=acceleration*dt;
            // (3) multiply final result by the damping factor to sim drag
            velocity*=damping;
            
            
            turbulance = ofVec3f(ofRandom(-1, 1),ofRandom(-1, 1),ofRandom(-1, 1));
            
            acceleration = gravity + landerForces + turbulance;
            
            angle = angle + angVelocity*dt;
            lander.setRotation(0, angle, 0, 1, 0);
            angVelocity = angVelocity+(angAcceleration_clockwise + angAcceleration_counterClockwise)*dt;
            angVelocity*=damping;
            
            landerForces = ofVec3f(0,0,0);
        }
        
        // The above ground level is object height minus the terrain height
        // We can intersect a ray from the bottom on the lander with the terrain, and
        // calculate the difference in y value of the lander and selected point
        Ray ray = Ray(Vector3(lander.getPosition().x,landerBounds.min().y(),lander.getPosition().z),Vector3(0,-1,0));
        if(octree.intersect(ray, octree.root, landerHoverNode)) {
            selectedPoint=octree.mesh.getVertex(landerHoverNode.points[0]);
            bPointSelected=true;
            AGL = landerBounds.min().y() - selectedPoint.y;
        }
    }
}

// load vertex buffer in preparation for rendering
//
void ofApp::loadVbo() {
    if (emitter->sys->particles.size() < 1 && explosionEmitter->sys->particles.size() < 1) return;

    vector<ofVec3f> sizes;
    vector<ofVec3f> points;
    for (int i = 0; i < emitter->sys->particles.size(); i++) {
        points.push_back(emitter->sys->particles[i].position);
        sizes.push_back(ofVec3f(emitter->sys->particles[i].radius)); // radius
    }
    
    for (int i =0; i< explosionEmitter->sys->particles.size(); i++) {
        points.push_back(explosionEmitter->sys->particles[i].position);
        sizes.push_back(ofVec3f(explosionEmitter->sys->particles[i].radius)); // radius
    }
    // upload the data to the vbo
    //
    int total = (int)points.size();
    vbo.clear();
    vbo.setVertexData(&points[0], total, GL_STATIC_DRAW);
    vbo.setNormalData(&sizes[0], total, GL_STATIC_DRAW);
}
