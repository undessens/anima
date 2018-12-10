

class video_effect:
	def __init__(self, _name, _midiChannel, _oscAddress, ):
		self.name = _name
		self.midiChannel = _midiChannel
		self.oscAddress = _oscAddress
		self.currentValue = 63
		self.isModified = False

	def printResult(self):
		print self.name + " : "+str(self.currentValue)

	def setValue(self, newVal):
		if(newVal>=0 and newVal <= 127):
			self.currentValue = newVal
			self.isModified = True


	def update(self):
		#Can smooth the final value send in OSC, using an easing method
		if self.isModified:
			self.isModified = False
			return self.currentValue
		else :
			return None

