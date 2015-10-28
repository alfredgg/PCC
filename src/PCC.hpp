/*
 * PCCDataStructures.hpp
 *
 *  Created on: Oct 20, 2015
 *      Author: alfred
 */

#ifndef SRC_PCC_HPP_
#define SRC_PCC_HPP_

#include "ofxNetwork.h"
#include "ofMain.h"
#include "ofxBox2d.h"

class ColorReceivedMessage {
public:
	ofColor color;
	string id;
	ofColor discretizedColor;
	float position;

	ColorReceivedMessage() {
	}

	ColorReceivedMessage(string _id, ofColor _color, ofColor _discretizedColor,
			float _position) {
		id = _id;
		color = _color;
		discretizedColor = _discretizedColor;
		position = _position;
	}
};

class TravellingCircle;

class DestroyCircleData {
public:
	TravellingCircle* t_circle;
	ofxBox2dCircle* b_circle;

	DestroyCircleData(TravellingCircle* _t_circle, ofxBox2dCircle* _b_circle) {
		t_circle = _t_circle;
		b_circle = _b_circle;
	}
};

class TravellingCircle {
private:
	ofVec3f iniVelocity;

	void drawNoisyLine() {
		ofPoint p1 = from, p2 = box2dCircle->getPosition();

		float angle = ofPoint(0.1, 0).angle(p2 - p1);
		int size = p1.distance(p2);
		if (p2.y < p1.y)
			angle = -angle;

		ofPoint p;
		ofNoFill();
		ofSetLineWidth(noisyLineWidth);
		ofBeginShape();
		for (int i = 0; i < size; ++i) {
			float noise = ofNoise(i / 10.0);
			p = ofPoint(i, ofMap(noise, 0, 1, -3.0 / 2, 3.0 / 2));
			p.rotate(0, 0, angle);
			p += p1;
			ofVertex(p.x, p.y);
		}
		ofEndShape();
	}

public:
	float strokeSize = 2, pathSize = 400, minMoveDistance = 0.5;
	ofColor circleColor, trackColor;
	ofPoint from;
	ofxBox2dCircle* box2dCircle = 0;
	unsigned char iniAlpha = 0;
	float startedAt = 0;
	bool enabled = true;
	ofEvent<DestroyCircleData> diedCircle;
	static float fullyVisibleTime;
	static float fadeTime;

	float static constexpr launchForce = 25.0f;
	float static constexpr innerSize = 5;
	float static constexpr externalSize = innerSize * 2;
	int static const noisyLineWidth = 2;

	TravellingCircle() {
	}

	TravellingCircle(ofColor _circleColor) {
		circleColor = _circleColor;
		trackColor = _circleColor;
		startedAt = ofGetElapsedTimef();
		iniAlpha = circleColor.a;
	}

	void launch(ofPoint direction, ofPoint _from) {
		direction *= launchForce;
		from = _from;
		box2dCircle->setVelocity(direction);
		box2dCircle->setFixedRotation(true);
		iniVelocity = direction;
	}

	void update() {
		if (!enabled)
			return;

		if (box2dCircle->getB2DPosition().distance(box2dCircle->getB2DPosition() + box2dCircle->getVelocity()) > minMoveDistance)
			box2dCircle->setVelocity(box2dCircle->getVelocity() * 0.97f);

		float timeAlive = startedAt + fullyVisibleTime;
		float remainingTimeAlive = timeAlive - ofGetElapsedTimef();
		bool dying = remainingTimeAlive < 0;

		if (dying) {
			float remainingTimeVisible = remainingTimeAlive + fadeTime;
			bool shouldBeRemoved = remainingTimeVisible < 0;
			if (shouldBeRemoved) {
				circleColor.a = 0;
				auto dcd = DestroyCircleData(this, box2dCircle);
				ofNotifyEvent(diedCircle, dcd, this);
				enabled = false;
				return;
			}
			circleColor.a = (remainingTimeVisible / fadeTime) * iniAlpha;
		}
	}

	void draw() {
		if (box2dCircle == 0)
			return;
		ofSetColor(circleColor);
		drawNoisyLine();
		ofNoFill();
		ofSetLineWidth(strokeSize);
		ofCircle(box2dCircle->getPosition().x, box2dCircle->getPosition().y,
				externalSize + strokeSize);
		ofFill();
		ofCircle(box2dCircle->getPosition().x, box2dCircle->getPosition().y,
				innerSize);
	}
};

class AcceptedColor {
public:
	ofColor color, representation;
	float threshold;
	bool isValid = false;

	AcceptedColor() {
	}

	AcceptedColor(string _color, float _threshold, string _representation) {
		threshold = _threshold;
		color = fromString(_color);
		representation = fromString(_representation);
		isValid = true;
	}

	ofColor fromString(string _color) {
		auto s_color = ofSplitString(_color, ",");
		if (s_color.size() != 3)
			return ofColor();
		return ofColor(ofToInt(s_color[0]), ofToInt(s_color[1]),
				ofToInt(s_color[2]));
	}
};

class LauncherCircle {
private:
	int static const n_waves = 50;
	float next_sec_wave = 0;
	array<float, n_waves> waves;
	float nextTimeLaunch = 0;

public:

	float static constexpr waves_velocity = 0.5f;
	float static constexpr waves_ratio = 2.0f;
	float static constexpr innerSize = 50;
	float static constexpr externalSize = 60;

	float minAngle, maxAngle, currentSinValue = 0;
	ofColor innerColor = ofColor(240, 240, 240, 0), externalColor = ofColor(215,
			185, 140, 0), waveColor = ofColor(255, 222, 166, 0);
	ofPoint position;
	bool valid = false;
	static float marginInLaunches;

	LauncherCircle() {
	}

	LauncherCircle(string _position, string _innerColor, string _externalColor,
			string _waveColor, float _minAngle, float _maxAngle) {
		auto s_pos = ofSplitString(_position, ",");
		if (s_pos.size() != 2)
			position = ofPoint(0, 0);
		else
			position = ofPoint(ofToInt(s_pos[0]), ofToInt(s_pos[1]));

		auto s_color1 = ofSplitString(_innerColor, ",");
		if (s_color1.size() == 4)
			innerColor = ofColor(ofToInt(s_color1[0]), ofToInt(s_color1[1]),
					ofToInt(s_color1[2]), ofToInt(s_color1[3]));
		auto s_color2 = ofSplitString(_externalColor, ",");
		if (s_color2.size() == 4)
			externalColor = ofColor(ofToInt(s_color2[0]), ofToInt(s_color2[1]),
					ofToInt(s_color2[2]), ofToInt(s_color2[3]));
		auto s_color3 = ofSplitString(_waveColor, ",");
		if (s_color3.size() == 4)
			waveColor = ofColor(ofToInt(s_color3[0]), ofToInt(s_color3[1]),
					ofToInt(s_color3[2]), ofToInt(s_color3[3]));

		minAngle = _minAngle;
		maxAngle = _maxAngle;

		for (int i = 0; i < n_waves; i++)
			waves[i] = 0;

		valid = true;
	}

	void draw(bool showAngleRange) {
		auto value = sin(currentSinValue);
		auto currentSize = ofMap(value, -1, 1, innerSize - 4, innerSize);
		currentSinValue += 0.05;
		if (currentSinValue > 100)
			currentSinValue = 0;

		ofSetColor(externalColor);
		ofCircle(position.x, position.y, externalSize);
		ofSetColor(innerColor);
		ofCircle(position.x, position.y, currentSize);

		ofNoFill();
		ofSetLineWidth(1);
		ofSetColor(waveColor);
		for (auto& wave_size : waves)
			if (wave_size != 0)
				ofCircle(position.x, position.y, wave_size);
		ofFill();

		if (!showAngleRange)
			return;

		ofSetColor(ofColor::black);
		ofSetLineWidth(2);
		auto step = (maxAngle - minAngle) / 10;
		for (int i = 0; i < 10; i++) {
			auto angle = minAngle + step * i;
			auto point1 = ofPoint(cos(angle * DEG_TO_RAD),
					sin(angle * DEG_TO_RAD));
			angle += step;
			auto point2 = ofPoint(cos(angle * DEG_TO_RAD),
					sin(angle * DEG_TO_RAD));
			point1 *= (externalSize + 5);
			point2 *= (externalSize + 5);
			ofLine(point1.x + position.x, point1.y + position.y,
					point2.x + position.x, point2.y + position.y);
		}
	}

	void update() {
		if (next_sec_wave < ofGetElapsedTimef()) {
			for (int i = 0; i < n_waves; i++) {
				if (waves[i] == 0) {
					waves[i] = externalSize;
					break;
				}
			}
			next_sec_wave = ofGetElapsedTimef() + waves_ratio;
		}
		for (auto& wave : waves) {
			if (wave != 0) {
				wave += waves_velocity;
				if (wave > ofGetWidth() * 2)
					wave = 0;
			}
		}
	}

	bool canLaunch() {
		return ofGetElapsedTimef() > nextTimeLaunch;
	}

	TravellingCircle launch(ofColor color) {
		nextTimeLaunch = ofGetElapsedTimef() + marginInLaunches;
		return TravellingCircle(color);
	}

	ofPoint getDirection(float _position) {
		float angle = ((maxAngle - minAngle) * _position) + minAngle;
		return ofPoint(cos(angle * DEG_TO_RAD), sin(angle * DEG_TO_RAD));
	}

	ofPoint getTravellerPosition(ofPoint direction) {
		return (direction * externalSize + TravellingCircle::launchForce)
				+ position;
	}
};

class ColorReceiver {
private:
	ofxUDPManager udpConnection;
	bool created = false;

public:
	ofEvent<ColorReceivedMessage> colorReception;
	vector<AcceptedColor> acceptedColors;
	vector<string> cranes;
	map<string, float> nextAccepted;

	ColorReceiver() {
	}

	void create(int port) {
		udpConnection.Create();
		udpConnection.Bind(port);
		udpConnection.SetNonBlocking(true);
		created = true;
	}

	float distance(ofColor &color1, ofColor &color2) {
		int r = color1.r - color2.r;
		int g = color1.g - color2.g;
		int b = color1.b - color2.b;
		return sqrt(r * r + g * g + b * b);
	}

	int closedColorIdx(ofColor &color) {
		unsigned int idx = -1;
		float closest_dist = 10000;
		int dist = 0;
		AcceptedColor t_color;

		for (unsigned int i = 0; i < acceptedColors.size(); ++i) {
			t_color = acceptedColors[i];
			dist = distance(color, t_color.color);
			if ((dist < t_color.threshold) && (dist < closest_dist)) {
				closest_dist = dist;
				idx = i;
			}
		}
		return idx;
	}

	int craneIndex(string _identifier) {
		for (unsigned int i = 0; i < cranes.size(); ++i)
			if (cranes[i] == _identifier)
				return i;
		cranes.push_back(_identifier);
		ofLogNotice() << "New crane found: " << _identifier;
		return cranes.size() - 1;
	}

	void receive() {
		if (!created)
			return;

		char udpMessage[10000];
		udpConnection.Receive(udpMessage, 10000);
		std::string message = udpMessage;

		if (message.length() <= 1)
			return;
		auto ssplit = ofSplitString(message, ":");

		if (ssplit.size() != 6)
			return;
		ofColor c_received(ofToFloat(ssplit[1]), ofToFloat(ssplit[2]),
				ofToFloat(ssplit[3]));
		int idx = closedColorIdx(c_received);
		if (idx == -1)
			return;
		auto c_discrete = acceptedColors[idx];

		float position = ofToFloat(ssplit[5]);

		ColorReceivedMessage crm(ssplit[0], c_discrete.representation,
				c_discrete.color, position);
		ofNotifyEvent(colorReception, crm, this);
	}
};

class Physics {
private:
	vector<ofxBox2dCircle*> circles;

	ofxBox2dCircle* createBody(float density, ofPoint position, float radius) {
		circles.push_back(new ofxBox2dCircle);
		circles.back()->setPhysics(density, 1, 0);
		circles.back()->setup(box2d.getWorld(), position.x, position.y, radius);

		return circles.back();
	}

public:
	ofxBox2d box2d;
	void create() {
		box2d.init();
		box2d.setGravity(0, 0);
		box2d.setFPS(60.0);
		box2d.createBounds();
	}

	void update() {
		box2d.update();
	}

	void draw() {
		for (auto circle : circles)
			circle->draw();
	}

	void addLauncher(LauncherCircle& circle) {
		createBody(10000000.0, circle.position, circle.externalSize);
	}

	void addTravelling(TravellingCircle* circle, ofPoint position) {
		auto body = createBody(1.0, position,
				circle->externalSize + circle->strokeSize);
		circle->box2dCircle = body;
	}

	void destroyCircle(ofxBox2dCircle* circle) {
		box2d.getWorld()->DestroyBody(circle->body);
		int idx = -1;
		for (unsigned int i = 0; i < circles.size(); i++) {
			if (circles[i] == circle) {
				idx = i;
				break;
			}
		}
		if (idx != -1)
			circles.erase(circles.begin() + idx);
	}
};

#endif /* SRC_PCC_HPP_ */
