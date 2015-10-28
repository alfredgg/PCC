#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "PCC.hpp"

class ofApp: public ofBaseApp {
	ColorReceiver creceiver;
	ofxOscSender sender;
	vector<TravellingCircle> circles;
	map<string, LauncherCircle> launchers;
	Physics physics;

	bool showPhysics = false;
	bool showAngleRange = false;
	float nextTimeGC = 0;

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void configure();

	void colorReceived(ColorReceivedMessage &data);
	void removeCircle(DestroyCircleData &data);
	void GarbageCollector();
};
