import cv2 as cv
import numpy as np
import paho.mqtt.client as mqtt
import time

def nothing(x):
    pass
    
def on_publish(client, userdata, mid):
    print("message published")
    
lastMsg = time.time()*1000    
    
client = mqtt.Client("rpi_client2212") 
client.on_publish = on_publish
client.connect("broker.mqttdashboard.com",1883)
client.loop_start()


class GUI:
	def __init__(self):
		self.cameraWindow = "cameraWindow"
		self.thresholdWindow = "thresholdWindow"
		self.settingsWindow = "settingsWindow"
			
		cv.namedWindow(self.cameraWindow);
		cv.namedWindow(self.thresholdWindow);
		cv.namedWindow(self.settingsWindow);
			
		lowH=40
		lowS=50
		lowV=40
		highH = 80
		highS=255
		highV=255
		blurKernelSize = 2;
		roundness = 70;

		cv.createTrackbar('lowH', self.settingsWindow, lowH, 180, nothing)
		cv.createTrackbar('highH', self.settingsWindow, highH, 180, nothing)
		cv.createTrackbar('lowS', self.settingsWindow, lowS, 255, nothing)
		cv.createTrackbar('highS', self.settingsWindow, highS, 255, nothing)
		cv.createTrackbar('lowV', self.settingsWindow, lowV, 255, nothing)
		cv.createTrackbar('highV', self.settingsWindow, highV, 255, nothing)
		cv.createTrackbar('blurKernelSize', self.settingsWindow, blurKernelSize, 50, nothing)
		cv.createTrackbar('roundness%', self.settingsWindow, roundness, 100, nothing)


	def lowH(self):
		return cv.getTrackbarPos('lowH', self.settingsWindow)
		
	def highH(self):
		return cv.getTrackbarPos('highH', self.settingsWindow)

	def lowS(self):
		return cv.getTrackbarPos('lowS', self.settingsWindow)
		
	def highS(self):
		return cv.getTrackbarPos('highS', self.settingsWindow)
		
	def lowV(self):
		return cv.getTrackbarPos('lowV', self.settingsWindow)
		
	def highV(self):
		return cv.getTrackbarPos('highV', self.settingsWindow)
		
	def blurKernelSize(self):
		return cv.getTrackbarPos('blurKernelSize', self.settingsWindow)
		
	def roundness(self):
		return cv.getTrackbarPos('roundness%', self.settingsWindow)
				
	def showCameraImage(self, image):
		cv.imshow(self.cameraWindow, image)
		
	def showThresholdImage(self, image):
		cv.imshow(self.thresholdWindow, image);
		
	def __del__(self):
		cv.destroyAllWindows()
			
			
class CameraModule:
	def __init__(self):
		self.cameraWidth = 400	# ZMNIEJSZYC W PRZYPADKU PROBLEMOW Z WYDAJNOSCIA
		self.cameraHeight = 300
		
		self.cam = cv.VideoCapture(0)
		self.cam.set(cv.CAP_PROP_FRAME_WIDTH, self.cameraWidth)
		self.cam.set(cv.CAP_PROP_FRAME_HEIGHT, self.cameraHeight)
		
		self.lowH=40
		self.lowS=50
		self.lowV=40
		self.highH = 80
		self.highS=255
		self.highV=255
		self.blurKernelSize = 2;
		self.roundness = 70;
			
	def setParameters(self, lowH, lowS, lowV, highH, highS, highV, blurKernelSize, roundness):
		self.lowH=lowH
		self.lowS=lowS
		self.lowV=lowV
		self.highH = highH
		self.highS=highS
		self.highV=highV
		self.blurKernelSize = blurKernelSize;
		self.roundness = roundness;
		
	def searchForTarget(self):
		ret, self.imageFromCamera = self.cam.read()
			
		if self.blurKernelSize > 0:
			self.imageFromCamera = cv.blur(self.imageFromCamera, (self.blurKernelSize, self.blurKernelSize))
	
		imageHSV = cv.cvtColor(self.imageFromCamera, cv.COLOR_BGR2HSV)
	
		low = (self.lowH, self.lowS, self.lowV)
		high = (self.highH, self.highS, self.highV)
		self.colorThreshold = cv.inRange(imageHSV, low, high)
	
		rows = self.colorThreshold.shape[0]
		circles=cv.HoughCircles(self.colorThreshold, cv.HOUGH_GRADIENT_ALT, 1.5, 10, 300, self.roundness / 100.0, 0, 0);
	
		return circles
		
	def getImageFromCamera(self):
		return self.imageFromCamera
		
	def getThresholdImage(self):
		return self.colorThreshold

	def __del__(self):
		self.cam.release()

#MAIN LOOP ============================================================

camera = CameraModule()
gui = GUI()

while True:
	camera.setParameters(gui.lowH(), gui.lowS(), gui.lowV(), gui.highH(), gui.highS(), gui.highV(), gui.blurKernelSize(), gui.roundness())
	
	circles = camera.searchForTarget()
	cameraImage = camera.getImageFromCamera()


	height, width, _ = cameraImage.shape
	
	
	
	if circles is not None:
		circles = np.uint16(np.around(circles))
		
		largestCircle = circles[0, 0]
		
		for i in circles[0, :]:
			if i[2] > largestCircle[2]:
				largestCircle = i
			
		center = (largestCircle[0], largestCircle[1])
		error = width/2 - center[0]
		errorPercent = int(error / (width/2)*100)
	# Wysylanie komendy
		linera_velocity = 70
		omega =  1 * errorPercent 
		now = time.time()*1000
		if now - lastMsg > 50:
			lastMsg = now
			try:
				msg =str(f"{linera_velocity:};{omega:}")
				pubMsg = client.publish(
					topic="PUM2023v2/BETA/CTR",
					payload=msg.encode('utf-8'),
					qos=0,
				)
				pubMsg.wait_for_publish()
				print(pubMsg.is_published())
    
			except Exception as e:
				print(e)
		

		cv.circle(cameraImage, center, 1, (0, 100, 100), 3)					#najwiekszy okrag
		cv.circle(cameraImage, center, largestCircle[2], (255, 0, 255), 3)
		
		x1, y1 = int(width/2), int(center[1])	#linia laczaca okrag ze srodkiem obrazu
		x2, y2 = int(center[0]), int(center[1])
		cv.line(cameraImage, (x1, y1), (x2, y2), (0, 255, 0), 3)
		
		x1, y1 = int(width/2), int(height-1)	#srodek obrazu
		x2, y2 = int(width/2), int(0)
		cv.line(cameraImage, (x1, y1), (x2, y2), (0, 255, 0), 3)
		cv.putText(cameraImage, str(errorPercent)+'%', (int(width/4),int(height/4)), cv.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2, cv.LINE_AA)
		

	gui.showCameraImage(cameraImage)
	gui.showThresholdImage(camera.getThresholdImage())
	
	k = cv.waitKey(10)
	if k != -1:
		break

