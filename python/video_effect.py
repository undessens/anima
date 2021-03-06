

class video_effect:
	def __init__(self, _name, _midiChannel, _oscAddress, mapFunc = None):
		self.name = _name
		self.midiChannel = _midiChannel
		self.oscAddress = _oscAddress
		self.currentValue = 63
		self.isModified = False
		self.mapFunc = mapFunc or (lambda x:x)

	def printResult(self):
		print (self.name + " : "+str(self.currentValue))

	def setValue(self, newVal):
		if(newVal>=0 and newVal <= 127):
			self.currentValue = newVal
			self.isModified = True

	def getMappedValue(self):
		return self.mapFunc(self.currentValue)
		
	def update(self):
		#Can smooth the final value send in OSC, using an easing method
		if self.isModified:
			self.isModified = False
			return self.mapFunc(self.currentValue)
		else :
			return None

