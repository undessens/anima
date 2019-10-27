#include "ofxHTTP.h"

class StreamVid {
public:
  ~StreamVid() {
    vidSender.waitForThread( true);
  }
  void setup(float tFPS = 3.0) {
    newFrameToSend = false;
    ofxHTTP::SimpleIPVideoServerSettings settings;

    // Many other settings are available.
    settings.setPort(7890);

    // The default maximum number of client connections is 5.
    settings.ipVideoRouteSettings.setMaxClientConnections(1);
    settings.ipVideoRouteSettings.setMaxClientQueueSize(1);
    settings.ipVideoRouteSettings.setMaxClientFrameRate(tFPS);

    // Apply the settings.
    server.setup(settings);

    // Start the server.
    server.start();
    targetPeriod = tFPS > 0 ? 1.0 / tFPS : 0.0;
    imUpdateT = ofGetElapsedTimef();

    vidSender.setup(this);
    setStreamState(false);
  }

  void setStreamState(bool s) {
    doStream = s;
    if (doStream != vidSender.isThreadRunning()) {
      if (doStream)vidSender.startThread();
      else vidSender.stopThread();
    }
  }
  const ofPixels & getPixels() {
    return img.getPixels();
  }

  void publishScreen() {
    if (!doStream) {return;}
    float newT = ofGetElapsedTimef();
    if (targetPeriod == 0 || newT - imUpdateT > targetPeriod) {
      img.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
      imUpdateT = newT;
      newFrameToSend = true;

    }
  }

private:
  ofxHTTP::SimpleIPVideoServer server;
  ofImage img;
  ofPixels pixs;
  std::atomic<float> imUpdateT;
  float targetPeriod;
  bool doStream;

  class VidSender : public ofThread {
  public:
    void setup(StreamVid * o) {
      owner = o;
      setThreadName("vidSender");
      sentFrameNum = 0;

    }
    void threadedFunction() final{
      while (isThreadRunning()) {
        bool hasSentFrame = false;
        if (owner->newFrameToSend ) {
          hasSentFrame = true;
          owner->server.send(owner->getPixels());
          owner->newFrameToSend = false;
          sentFrameNum++;
        }
        if (owner->targetPeriod > 0.0) {
          float curT = ofGetElapsedTimef();
          float nextT = owner->imUpdateT + owner->targetPeriod;
          const float minSleepConfidence = 5.0 / 1000.0;
          float sleepTime = nextT - curT ;
          // if(hasSentFrame )DBG("sleep to next " << ofToString(sleepTime));
          // if (sleepTime <= 0)DBG("over sleeping " << ofToString(sleepTime));
          
          if (sleepTime > minSleepConfidence) {
            
            // if (sleepTime < minSleep) {
            //   sleepTime = minSleep;
            // }
            ofThread::sleep(sleepTime * 1000.0);
          }
          else{
            ofThread::yield();
          }
        }

      }
    }
    long long sentFrameNum ;
    StreamVid * owner;


  };


  std::atomic<bool> newFrameToSend;
  VidSender vidSender;

};