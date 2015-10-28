#include "ofApp.h"
#include "ofxJSON.h"

float TravellingCircle::fullyVisibleTime = 5.0f;
float TravellingCircle::fadeTime = 5.0f;
float LauncherCircle::marginInLaunches = 0.25f;

//--------------------------------------------------------------
void ofApp::setup() {
	ofLog() << "Starting application";

	ofBackground(ofColor(244, 235, 220));
	ofSetCircleResolution(50);
	ofSetVerticalSync(true);
	ofSetFrameRate(60);

	physics.create();
	configure();
}

void ofApp::configure() {
	ofxJSONElement config;
	bool parsingSuccessful = config.open("settings.json");
	if (parsingSuccessful) {
		LauncherCircle::marginInLaunches = config["secs_between_launches"].asFloat();
		TravellingCircle::fullyVisibleTime = config["secs_traveller_fully_visible"].asFloat();
		TravellingCircle::fadeTime = config["secs_traveller_fade"].asFloat();

		auto senderConfig = config["sender"];
		sender.setup(senderConfig["ip"].asString(),
				senderConfig["port"].asInt());
		ofLogNotice() << "OSC Sender created for ip "
				<< senderConfig["ip"].asString() << " in port "
				<< senderConfig["port"].asInt();

		auto receiverConfig = config["receiver"];
		creceiver.create(receiverConfig["port"].asInt());
		ofAddListener(creceiver.colorReception, this, &ofApp::colorReceived);
		ofLogNotice() << "UDP Receiver created at port "
				<< receiverConfig["port"].asInt();

		auto colors = config["colors"];
		for (auto& color : colors) {
			AcceptedColor ac(color["rgb"].asString(),
					color["threshold"].asInt(),
					color["representation"].asString());
			if (ac.isValid) {
				creceiver.acceptedColors.push_back(ac);
				ofLog() << "Color accepted: " << ac.color << ". Shown as "
						<< ac.representation;
			} else
				ofLogError() << "Color " << color["rgb"].asString()
						<< " is not valid";
		}

		auto cnf_launchers = config["launchers"];
		for (auto& launcher : cnf_launchers) {
			auto identifier = launcher["id"].asString();
			LauncherCircle launcherc(launcher["position"].asString(),
					launcher["rgb"].asString(), launcher["border"].asString(),
					launcher["wave"].asString(),
					launcher["min_angle"].asFloat(),
					launcher["max_angle"].asFloat());
			physics.addLauncher(launcherc);
			launchers[identifier] = launcherc;
			ofLog() << "Launcher added at " << launcherc.position
					<< " with id \"" << identifier << "\" and angles: ["
					<< launcherc.minAngle << ", " << launcherc.maxAngle << "]";
		}
	} else {
		ofLogFatalError()
				<< "Settings file (settings.json in data folder) not found or has a wrong format.";
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	creceiver.receive();
	physics.update();
	for (auto& launcher : launchers)
		launcher.second.update();
	for (auto& circle : circles)
		circle.update();
	if (ofGetElapsedTimef() > nextTimeGC)
		GarbageCollector();
}

//--------------------------------------------------------------
void ofApp::draw() {
	for (auto& circle : circles)
		circle.draw();
	for (auto& launcher : launchers)
		launcher.second.draw(showAngleRange);
	if (showPhysics)
		physics.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
	if ((key >= '0') && (key <= '9')) {
		char letter = (char) key;
		std::string str(&letter, &letter + 1);
		float value = ofToFloat(str) / 10;
		ColorReceivedMessage crm("1", ofColor::gray, ofColor::gray, value);
		colorReceived(crm);
	} else if (key == 'a')
		showAngleRange = !showAngleRange;
	else if (key == 'f')
		ofToggleFullscreen();
	else if (key == 'p')
		showPhysics = !showPhysics;
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

void ofApp::colorReceived(ColorReceivedMessage &data) {
	ofxOscMessage m;
	m.setAddress("/color");
	m.addFloatArg(data.discretizedColor.r / 255.0);
	m.addFloatArg(data.discretizedColor.g / 255.0);
	m.addFloatArg(data.discretizedColor.b / 255.0);
	sender.sendMessage(m);

	auto launcher = &launchers[data.id];
	if (launcher->valid && launcher->canLaunch()) {
		auto circle = launcher->launch(data.color);
		circles.push_back(circle);
		auto direction = launcher->getDirection(data.position);
		auto position = launcher->getTravellerPosition(direction);
		physics.addTravelling(&circles.back(), position);
		circles.back().launch(direction, launcher->position);
		ofAddListener(circles.back().diedCircle, this, &ofApp::removeCircle);
	}
}

void ofApp::removeCircle(DestroyCircleData &data) {
	physics.destroyCircle(data.b_circle);
	data.t_circle->box2dCircle = 0;
}

void ofApp::GarbageCollector() {
	vector<int> garbage;

	int idx = -1;
	for (auto& circle : circles) {
		idx++;
		if (circle.enabled)
			continue;
		if (circle.box2dCircle != 0) {
			physics.destroyCircle(circle.box2dCircle);
			circle.box2dCircle = 0;
		}
		garbage.push_back(idx);
	}

	for (int i = garbage.size() - 1; i >= 0; i--) {
		idx = garbage[i];
		circles.erase(circles.begin() + idx);
	}
	nextTimeGC = ofGetElapsedTimef() + 5;
}
