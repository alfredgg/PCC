import socket
import argparse
from random import choice
from time import sleep

parser = argparse.ArgumentParser()
parser.add_argument('--ip', default='127.0.0.1')
parser.add_argument('--port', type=int, default=8000)
parser.add_argument('--seconds', type=float, default=1)
parser.add_argument('--steps', type=int, default=5)
parser.add_argument('--id', type=str, default='1')
parser.add_argument('--show', type=int, default=1)
args = parser.parse_args()

COLORS = ['255,0,0', '0,255,0', '0,0,255']

position = 0
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
	color = choice(COLORS).split(',')
	msg = '%s:%s:%s:%s:%1.3f:%1.3f' % (args.id, color[0], color[1], color[2], 0.5, position/100.0)
	sock.sendto(msg, (args.ip, args.port))
	if args.show:
		print msg
	sleep(args.seconds)
	position += args.steps
	position = position % 100

