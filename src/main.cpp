#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	//ofSetupOpenGL(1280,720,OF_WINDOW);
	//ofSetupOpenGL(2560,1600,OF_WINDOW); //macbook display size
	//ofSetupOpenGL(1920,1080,OF_WINDOW);
	ofSetupOpenGL(1080,1920,OF_WINDOW);			// <-------- setup the GL context

	ofRunApp(new ofApp());
}
