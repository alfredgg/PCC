import numpy as np
import cv2
import argparse
import uuid
import socket
import time

parser = argparse.ArgumentParser()
parser.add_argument('--ip', default='127.0.0.1', help='The ip of the OSC server')
parser.add_argument('--port', type=int, default=8000, help='The port the OSC server is listening on')
parser.add_argument('--capture', type=int, default=0, help='The cam identifier')
parser.add_argument('--show', type=int, default=0, help='Indicates if the frame is shown')
parser.add_argument('--roi', type=int, default=100, help='Indicates the ROI size in pixels')
parser.add_argument('--sampling', type=float, default=0.0, help='Sampling percentage (0..1) to obtain from ROI')
parser.add_argument('--delay', type=float, default=0, help='Delay time between frames')
parser.add_argument('--send', type=int, default=1, help='Indicates if it should send the color')
parser.add_argument('--identifier', type=str, default='', help='This is the identifier that will be sent to the visor')
parser.add_argument('--show_msg', type=int, default=0)
parser.add_argument('--servo', type=int, default=0, help='Indicates if the servo should be controlled')
args = parser.parse_args()

COLOR_SIZE = 100
SHOW_SAMPLE = [1, 0.9, 0.75, 0.5, 0.2, 0.1, 0.01, 0.001, 0.0001, 0.00001]
SERVO_1_SECS = 3.0
SERVO_1_LEFT = 1.6
SERVO_1_RIGHT = 1.3

cap = cv2.VideoCapture(args.capture)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
identifier = str(uuid.uuid4()) if not args.identifier else args.identifier

width, height = cap.get(cv2.cv.CV_CAP_PROP_FRAME_WIDTH), cap.get(cv2.cv.CV_CAP_PROP_FRAME_HEIGHT)
center_roi_x, center_roi_y = int(width / 2 - args.roi / 2), int(height / 2 - args.roi / 2)
pos_roi_x, pos_roi_y = int(width + (width/2-args.roi/2)), int(height/3-args.roi/2)
pos_color_x, pos_color_y = width + (width/2 - COLOR_SIZE / 2), (height / 3 * 2 - COLOR_SIZE/2)
n_pixels = width * height
sampling = int(n_pixels * args.sampling) if 0 < args.sampling < 1 else n_pixels
black_image = np.zeros((height, width * 2, 3), np.uint8)
color_image = np.zeros((COLOR_SIZE, COLOR_SIZE, 3), np.uint8)
shown_sample = [int(n_pixels * v) for v in SHOW_SAMPLE]
end_color_y = pos_color_y + COLOR_SIZE
control_servo = args.servo == 1
servo_position = 0
running = True

INI_SERVO_VALUE = 30
END_SERVO_VALUE = 130
STEP_SERVO = 2

print 
print 'Identifier: ' + identifier
print 'Captured image size: %d, %d' % (width, height)
print 'ROI size: %d, %d' % (args.roi, args.roi)
print 'Sending to %s:%d' % (args.ip, args.port)
print 'Controlling servo: %s' % ('yes' if control_servo else 'no')
print 'Showing results: %s' % ('yes' if args.show else 'no')
print 'Pixels in sent sampling: %d / %d' % (sampling, n_pixels)
if args.show:
    print 'Pixels in the shown samples: %s ' % repr(shown_sample)
print 'Delay: %s' % ('no' if args.delay == 0.0 else str(args.delay * 1000) + 'ms')
print

def sample(colorset, _size):
    idx_x = np.random.randint(colorset.shape[0], size=_size)
    idx_y = np.random.randint(colorset.shape[1], size=_size)
    return colorset[idx_y, idx_x, :]

def servo():
    import RPi.GPIO as GPIO

    GPIO.setmode(GPIO.BCM)
    GPIO.setup(18, GPIO.OUT)
    pwm = GPIO.PWM(18, 100)
    pwm.start(5)

    values = range(INI_SERVO_VALUE, END_SERVO_VALUE, STEP_SERVO)
    neg_values = values[:]
    neg_values.reverse()
    values.extend(neg_values)
    idx = 0
    while running:
        idx += 1
        idx = idx % len(values)
        pwm.ChangeDutyCycle(values[idx]/10.0)
        servo_position = values[idx] * 1.0 / (END_SERVO_VALUE - INI_SERVO_VALUE) 
        time.sleep(0.05)
    print 'Stopping servo'
    pwm.stop()
    GPIO.cleanup() 

if control_servo:
    from threading import Thread
    thread_servo = Thread(target=servo)
    thread_servo.start()

try:
	while(True):
	    ret, frame = cap.read()
	    if not ret:
	        continue

	    m_roi = frame[center_roi_y : center_roi_y + args.roi, center_roi_x : center_roi_x + args.roi]
	    if sampling == n_pixels:
	        color = cv2.mean(m_roi)[:3]
	    else:
	        sampling_pixels = sample(m_roi, sampling)
	        color = np.mean(sampling_pixels, axis=0)
	    luminance = (0.2126 * color[2] + 0.7152 * color[1] + 0.0722 * color[0]) / 255

	    if args.show:
	        black_image[:height, :width] = frame[:,:]        
	        black_image[pos_roi_y:pos_roi_y+args.roi, pos_roi_x:pos_roi_x+args.roi] = m_roi[:,:]
	        cv2.rectangle(black_image, (center_roi_x, center_roi_y), (center_roi_x + args.roi, center_roi_y + args.roi), (255, 0, 0))
	        black_image[pos_color_y:pos_color_y+COLOR_SIZE, pos_color_x:pos_color_x+COLOR_SIZE] = color
	        for i, n in enumerate(shown_sample):
	            pos_x, pos_y = int(width + (width / len(shown_sample) * i)), int(end_color_y + (height - end_color_y) / 2)
	            size_x, size_y = int(width / len(shown_sample)), int((height - pos_y) / 2)
	            sampling_pixels = sample(m_roi, n)
	            color = np.mean(sampling_pixels, axis=0)
	            black_image[pos_y:pos_y+size_y, pos_x:pos_x+size_x] = color
	            cv2.rectangle(black_image, (pos_x, pos_y), (pos_x + size_x, pos_y + size_y), (255, 255, 255))
	        cv2.imshow('Processed image', black_image)
	        if cv2.waitKey(1) & 0xFF == ord('q'):
	            running = False
	            break

	    if args.send:
	        msg = '%s:%d:%d:%d:%1.3f:%1.3f' % (identifier, color[2], color[1], color[0], luminance, servo_position)
	        sock.sendto(msg, (args.ip, args.port))
	        if args.show_msg:
	            print msg
	    if args.delay != 0.0:
	        time.sleep(args.delay)
except KeyboardInterrupt:
	running = False
finally:
	print 'Releasing cam'
	cap.release()
	cv2.destroyAllWindows()