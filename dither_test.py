import matplotlib.pyplot as plt
import math
import itertools

def signal():
	n = 0
	samples_per_period = 400
	periods = 0.5
	while n < samples_per_period * periods:
		yield math.cos(n * 2 * math.pi / samples_per_period) * 0.5 + 0.5
		n += 1

def dither(signal):
	error = 0
	quantize = lambda x: 0 if x < 0.5 else 1
	for x in signal:
		quantized = quantize(x + error)
		error += x - quantized
		yield quantized


plt.plot(list(dither(signal())))
plt.plot(list(signal()))

plt.show()
