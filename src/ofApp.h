#pragma once

#include "ofMain.h"
#include "ofxBox2d.h"
#include "ofxBox2dParticleSystem.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "ofxFlowTools.h"

using namespace ofxBox2dParticleSystem;
using namespace flowTools;

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		ofxBox2d box2d;
    vector <shared_ptr<ofxBox2dCircle>> contourCircles;
    ParticleSystem particleSystem;

    ofVideoGrabber vidGrabber;
    ofxCvColorImage colorImg;
    ofxCvGrayscaleImage grayImg;
    ofxCvGrayscaleImage grayBg;
    ofxCvGrayscaleImage grayDiff;
    ofImage renderImg;
    ofxCvContourFinder contourFinder;

    bool hasGravity = false;
    bool debugMode = false;
    bool bLearnBackground;
    float eTimef = 0.0;

    ofxFloatSlider threshold;
    ofxPanel gui;

    bool imgOn = false;
    vector		<shared_ptr<ofxBox2dCircle> >	circles;
    vector <ofPolyline> edgeLines;

    int rectSize = 16;
    int camWidth = 640;
    int camHeight = 480;
    //int cropW = 270;  //if camWidth is 320, 135
    int cropW = 640;

    int maxParticles = 15000;
    int particleRadius = 2;

    int intervalSec = 30;
    int bgCol = 255;

    ofSoundPlayer bgm;

    //FlowToolsConfig
    ofFbo cameraFbo;

    vector< ftFlow* >		flows;
    ftOpticalFlow			opticalFlow;
    ftVelocityBridgeFlow	velocityBridgeFlow;
    ftDensityBridgeFlow		densityBridgeFlow;
    ftTemperatureBridgeFlow temperatureBridgeFlow;
    ftFluidFlow				fluidFlow;
    ftParticleFlow			particleFlow;

    float deltaTime;
    float lastTime;

    int windowWidth, windowHeight, densityWidth, densityHeight, simulationWidth, simulationHeight;

    int sceneNum = 3;
    int scene = 0;
};