import requests
import json
import os

gitAPIURL = "https://api.github.com/repos/jean-emmanuel/open-stage-control/releases/latest"

scriptDir = os.path.dirname(os.path.realpath(__file__))
localOSCDir = scriptDir+"/bin/open-stage-control"

def getLatestV():
  params = dict()
  resp = requests.get(url=gitAPIURL, params=params)
  data = resp.json()
  if data:
    data["version"] = data["tag_name"][1:] #remove v
    return data
  else:
    raise Exception("can't access github server")

def getCurrentV():
  with open(localOSCDir+"/package.json","r") as fp:
    data = json.load(fp)
    if data:
      return data["version"]
  raise Exception("can't open local file")


if __name__ == "__main__":
  curV  = getCurrentV()
  latestVData = getLatestV()
  latestVNum = latestVData["version"]
  if curV != latestVNum:
    import zipfile, io,os,shutil
    import tempfile
    # from tempfile import TemporaryDirectory
    # with TemporaryDirectory() as tmpDir:
    tmpDir = tempfile.mkdtemp()
    print("new vesion available " ,latestVNum,"current : ",curV )
    zipFileName = "open-stage-control-%s-node.zip"%latestVNum
    dlUrl = "https://github.com/jean-emmanuel/open-stage-control/releases/download/v%s/%s"%(latestVNum,zipFileName)
    r = requests.get(dlUrl)
    z = zipfile.ZipFile(io.BytesIO(r.content))
    z.extractall(tmpDir)
    tmpZip = tmpDir+"/"+zipFileName
    dldedPath = os.path.dirname(tmpZip)+"/open-stage-control"
    os.rename (tmpZip[:-4],dldedPath)
    print (tmpDir,dldedPath)
    oldPath = os.path.dirname(localOSCDir)+"/open-stage-control"
    shutil.rmtree(oldPath)
    shutil.move (dldedPath,os.path.dirname(localOSCDir)+"/")
    
    
    

  
  
