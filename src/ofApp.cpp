#include "ofApp.h"

//Target: 1080 * 1920
//135, 240
//9: 16
//Mac: 2560*1600
//--------------------------------------------------------------
void ofApp::setup(){

  //ofBackground(0);
  //ofEnableBlendMode(OF_BLENDMODE_SCREEN);
	//ofEnableAlphaBlending();
  ofSetVerticalSync(true);
  ofLog(OF_LOG_NOTICE, "width:" + ofToString(ofGetWidth()));
  ofLog(OF_LOG_NOTICE, "height:" + ofToString(ofGetHeight()));

  //init GUI
  gui.setup();
  gui.add(threshold.setup("threshold", 40, 10, 255));

  //init VideoGrabber
  vidGrabber.setDeviceID(1);
  vidGrabber.setup(camWidth, camHeight, true);
  ofLog(OF_LOG_NOTICE, "vid width:" + ofToString(vidGrabber.getWidth()));
  ofLog(OF_LOG_NOTICE, "vid height:" + ofToString(vidGrabber.getHeight()));

  colorImg.allocate(camWidth ,camHeight);
  grayImg.allocate(cropW, camHeight);
  grayBg.allocate(cropW, camHeight);
  grayDiff.allocate(cropW, camHeight);

  renderImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR);
  bLearnBackground = true;

  //Init box2d
  box2d.init();
  box2d.setGravity(0, 0);
  box2d.enableEvents();
  box2d.createBounds(0, 0, cropW, colorImg.height);
  box2d.setFPS(30);
  box2d.checkBounds(true);

  //Init particleSystem
  particleSystem.init(box2d.getWorld());
  particleSystem.setMaxParticles(maxParticles);
  particleSystem.setRadius(particleRadius);

  for(int i=0; i<maxParticles; i++) {
    particleSystem.addParticle(colorImg.width/4 + ofRandom(-20, 20), ofRandom(-50, 50));
  }

  //Init sound
  bgm.load("ambient.wav");
  bgm.setLoop(true);
  bgm.play();

  //Init flowTools
  densityWidth = 1280;
  densityHeight = 720;

  simulationWidth = densityWidth / 2;
  simulationHeight= densityHeight / 2;

  windowWidth = ofGetWindowWidth();
  windowHeight = ofGetWindowHeight();

	opticalFlow.setup(simulationWidth, simulationHeight);
	velocityBridgeFlow.setup(simulationWidth, simulationHeight);
	densityBridgeFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);
	temperatureBridgeFlow.setup(simulationWidth, simulationHeight);
	fluidFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);
  fluidFlow.setSpeed(0.04);
  fluidFlow.setVorticity(0.4);
  fluidFlow.setDissipationVel(2);
  fluidFlow.setDissipationDen(0.2);
	particleFlow.setup(simulationWidth, simulationHeight, densityWidth, densityHeight);

	flows.push_back(&velocityBridgeFlow);
	flows.push_back(&densityBridgeFlow);
	flows.push_back(&temperatureBridgeFlow);
	flows.push_back(&fluidFlow);
	flows.push_back(&particleFlow);

  for (auto flow : flows) { flow->setVisualizationFieldSize(glm::vec2(simulationWidth / 2, simulationHeight / 2)); }

  cameraFbo.allocate(camWidth, camHeight);
  ftUtil::zero(cameraFbo);
}

//--------------------------------------------------------------
void ofApp::update(){
  float dt = 1.0 / max(ofGetFrameRate(), 1.f);
  eTimef = ofGetElapsedTimef();

  box2d.update();
  vidGrabber.update();


  if (vidGrabber.isFrameNew()) {
    cameraFbo.begin();
    vidGrabber.draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
    cameraFbo.end();
    opticalFlow.setInput(cameraFbo.getTexture());

    renderImg.setFromPixels(vidGrabber.getPixels());
    renderImg.mirror(false, true);
    renderImg.crop(camWidth/2-cropW/2, 0, cropW, camHeight);
    colorImg.setFromPixels(renderImg.getPixels());
    grayImg = colorImg;
    if (bLearnBackground) {
      grayBg = grayImg;
      bLearnBackground = false;
    }
    grayDiff.absDiff(grayBg, grayImg);
    grayDiff.threshold(threshold);
    contourFinder.findContours(grayDiff, 20, (colorImg.width*colorImg.height)/ 3, 10, false);
    for (int i = 0; i < contourCircles.size(); i++) contourCircles[i]->destroy();

    contourCircles.clear();
    edgeLines.clear();
    for (int i = 0; i < contourFinder.nBlobs; i++) {
      for (int j = 0; j < contourFinder.blobs[i].pts.size(); j+=4) {
        glm::vec2 pos = contourFinder.blobs[i].pts[j];
        auto circle = make_shared<ofxBox2dCircle>();
        circle->setup(box2d.getWorld(), pos.x, pos.y, 4);
        contourCircles.push_back(circle);
      }
    }
  }
  for(int i = 0; i< contourFinder.nBlobs; i++){
    ofPolyline line;
    for(int j =0; j<contourFinder.blobs[i].pts.size(); j++){
      line.addVertex(contourFinder.blobs[i].pts[j]);
    }
    edgeLines.push_back(line);
  }

	opticalFlow.update();
	
	velocityBridgeFlow.setVelocity(opticalFlow.getVelocity());
	velocityBridgeFlow.update(dt);
	densityBridgeFlow.setDensity(cameraFbo.getTexture());
	densityBridgeFlow.setVelocity(opticalFlow.getVelocity());
	densityBridgeFlow.update(dt);
	temperatureBridgeFlow.setDensity(cameraFbo.getTexture());
	temperatureBridgeFlow.setVelocity(opticalFlow.getVelocity());
	temperatureBridgeFlow.update(dt);

	fluidFlow.addVelocity(velocityBridgeFlow.getVelocity());
	fluidFlow.addDensity(densityBridgeFlow.getDensity());
	fluidFlow.addTemperature(temperatureBridgeFlow.getTemperature());
  fluidFlow.update(dt);
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofBackground(0);
  ofSetColor(255);
  bgCol = 255 * abs(sin(eTimef * 0.2));
  float brightness = ofMap((int)eTimef % intervalSec, 0, intervalSec-1, 0, 255);

  if (scene == 0) {
    ofPushMatrix();
    ofScale((float)ofGetWidth() / (float)colorImg.width, (float)ofGetHeight() / (float)colorImg.height);
    if (imgOn) colorImg.draw(0, 0);
    particleSystem.updateMesh();
    ofPushStyle();
    float hue = abs(sin(eTimef  * 0.2)) * 255;
    ofColor sCol = ofColor(0);
    //sCol.setHsb(140, 255, 255);
    sCol.setHsb(hue, 255, 255);
    ofSetColor(sCol);
    particleSystem.draw();

    ofColor hCol = ofColor(0);
    //hCol.setHsb(40, 255, 255);
    hCol.setHsb(abs(int(hue) - 100) % 255, 255, 255);
    ofSetColor(hCol);
    for(int cnt = 0; cnt< edgeLines.size(); cnt++) edgeLines[cnt].draw();
    ofPopStyle();
    /*
    contourFinder.draw();
    for (size_t i=0; i<contourCircles.size(); i++) contourCircles[i]->draw();
    */
    ofPopMatrix();
  } else if (scene == 1) {
    ofPushMatrix();
    ofScale((float)ofGetWidth() / (float)colorImg.width, (float)ofGetHeight() / (float)colorImg.height);
    float tFac = abs(sin(ofGetElapsedTimef()) * 0.8) + 1;
    ofPixels pixels = colorImg.getPixels();
    for (int j = 0; j < camHeight; j+=rectSize) {
      for (int i = 0; i < cropW; i+=rectSize) {
        unsigned char r = pixels[(j * cropW + i) * 3];
        unsigned char g = pixels[(j * cropW + i) * 3 + 1];
        unsigned char b = pixels[(j * cropW + i) * 3 + 2];

        ofPushStyle();
        float hue = (r + g + b) / 3 * tFac;
        ofColor rCol = ofColor(0);
        rCol.setHsb(hue, 200, 255);
        ofSetColor(rCol);
        ofDrawRectangle(i, j, rectSize, rectSize);
        ofPopStyle();

      }
    }
    ofPopMatrix();
  } else {
    ofClear(0, 0);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    //cameraFbo.draw(0, 0, windowWidth, windowHeight);
    fluidFlow.draw(0, 0, windowWidth, windowHeight);
    ofPopStyle();
  }

  if (debugMode) gui.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  switch (key) {
    case ' ':
      bLearnBackground = true;
      break;
    case 'd':
      debugMode = !debugMode;
      break;
    case 'i':
      imgOn = !imgOn;
      break;
    case 'b':
      if (bgCol == 255) {
        bgCol = 0;
      } else {
        bgCol = 255;
      }
      break;
    case 'g':
      if (hasGravity) {
        box2d.setGravity(0, 0);
      } else {
        box2d.setGravity(0, 2);
      }
      hasGravity = !hasGravity;
      break;
    case 'm':
      scene = (scene + 1) % sceneNum;
      break;
  }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}
