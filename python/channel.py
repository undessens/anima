###################################################################
# Channel Class :
# Able to control motion and record sequences of
# any length and play it in loop.
# 
###################################################################

##################################################################
#  Behaviour : 
#
# Can't play and record in the same time.
# -> Start recording : stop playing.
# Manual move from fader or potar while playing automatically stop
# -> isModified while isRecording automatically stop playing.
# Limit the serial communication. Send arduino message only is the currentValue
# is modified, or potar/fader is moved
# Update function return None or Value
# -> In midi.py, send serial message only if smth in returned
##################################################################


class channel:
	def __init__(self, _name, _midiChannel, _arduinoID, _recordButton):
		self.name = _name
		self.midiChannel = _midiChannel
		self.recordButton = _recordButton
		self.arduinoID = _arduinoID
		self.currentValue = 63
		self.isModified = True
		#Recording
		self.isRecording = False
		self.isPlaying = False
		self.list_of_value = []
		self.currentIndex = 0
		self.recordingStart = 0.0  #msec


	def printResult(self):
		print self.name + " : "+str(self.currentValue)

	def setValue(self, newVal):
		if(newVal>0 and newVal<127):
			self.currentValue = newVal
			self.isModified = True

	def update( self):
		#only recording
		if self.isRecording and not(self.isPlaying):
			self.list_of_value.append(self.currentValue)
			if self.isModified :
				self.isModified = False
				return self.currentValue
			else :
				return None

		#only playing
		if self.isPlaying and not(self.isRecording) :
			self.currentIndex+=1
			if self.isModified :
				#stop playing because of manual move
				self.isPlaying = False
				self.isModified = False
				return self.currentValue
			if self.currentIndex >= len(self.list_of_value):
				self.currentIndex = 0
			if self.list_of_value[self.currentIndex] != self.currentValue :
				self.currentValue = self.list_of_value[self.currentIndex]
				return self.currentValue
			else :
				return None
		
		#if nothing is playing or recording 
		if not(self.isPlaying) and not(self.isRecording) :
			if self.isModified :
				self.isModified = False
				return self.currentValue
			else :
				return None

			
	def playStop( self, isPlay ):
		if isPlay and len(self.list_of_value)>0 :
			
			if( not self.isPlaying):
			#if already playing : do nothing
				self.currentIndex = 0
				self.isPlaying = True

		else :
			self.isPlaying = False
			#print self.name+" stop"

	def startRecording( self):
		self.isPlaying = False
		del self.list_of_value[:]
		self.isRecording = True

	def stopRecording (self):
		self.isRecording = False
		self.currentIndex = 0

	def clear(self):
                self.isRecording = False
                self.currentIndex = 0
                self.isPlaying = False
		del self.list_of_value[:]
                











