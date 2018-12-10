

class serial_effect:
	def __init__(self, _name, _midiChannel, _arduinoID, _toogle ):
		self.name = _name
		self.midiChannel = _midiChannel
		self.arduinoID = _arduinoID
		self.currentValue = 0
		self.isModified = False
		self.toogle = _toogle

	def printResult(self):
		print self.name + " : "+str(self.currentValue)

	def setValue(self, newVal):
                # button to toogle
                if(self.toogle):
                        if(newVal>0):
                                if(self.currentValue==0):
                                        self.currentValue = 127
                                else:
                                        self.currentValue = 0
                                self.isModified = True
                # fader and analog value
                else:
                        if(newVal>=0 and newVal <= 127):
                                self.currentValue = newVal
                                self.isModified = True



	def update(self):
		#Can smooth the final value send in OSC, using an easing method
		if self.isModified :
			self.isModified = False
			return self.currentValue
		else :
			return None

