

class transport_effect:
	def __init__(self, _name, _midiChannel, _arduinoID ):
		self.name = _name
		self.midiChannel = _midiChannel
		self.arduinoID = _arduinoID
		self.currentValue = False
		self.isModified = False

	def printResult(self):
		print self.name + " : "+str(self.currentValue)

	def setValue(self, newVal):
		if(newVal==127):
			self.currentValue = True
			self.isModified = True
		else :
			self.currentValue = False


	def update(self):
		#Can smooth the final value send in OSC, using an easing method
		if self.isModified and self.currentValue:
			self.isModified = False
			return True
		else :
			return False

