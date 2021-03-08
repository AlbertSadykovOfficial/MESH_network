#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>

const char exampleMeshName[] PROGMEM = "MeshNode_";
const char exampleWiFiPassword[] PROGMEM = "ChangeThisWiFiPassword_TODO";

unsigned int requestNumber = 0;
unsigned int responseNumber = 0;

String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance);
transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance);
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance);

/* Create the mesh node object */
ESP8266WiFiMesh meshNode = ESP8266WiFiMesh(manageRequest, manageResponse, networkFilter, FPSTR(exampleWiFiPassword), FPSTR(exampleMeshName), "", true);

// Получить
String manageRequest(const String &request, ESP8266WiFiMesh &meshInstance) {

   Serial.println(F("|Request|"));
  /* Print out received message */
  Serial.print(F("DATA_TO_ME: "));
  Serial.println(request);
  send_message(request + " (translated by SLAVE)");
  String command = "Im not waiting commands, message has translated to next node";
  
  // String command = meshInstance.getMessage();
  Serial.print(F("DATA_FROM_ME: "));
  Serial.println(command);

  /* return a string to send back */
  return (command);
}

transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) 
{
  transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;

  /* Print out received message */
  Serial.println(F("|manageResponse|"));
   String command = meshInstance.getMessage();
  Serial.print(F("DATA_FROM_ME: "));
  Serial.println(command);
  Serial.print(F("DATA_TO_ME: "));
  Serial.println(response);

  // Our last request got a response, so time to create a new request.
  //meshInstance.setMessage(String(F("Hello world request #")) + String(++requestNumber) + String(F(" from "))
   //                       + meshInstance.getMeshName() + meshInstance.getNodeID() + String(F(".")));

  // (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
  return statusCode;
}

// ПОДКЛЮЧЕНИЕ?
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
  //Serial.println();
  //Serial.println("Networks:");
 for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex) 
 {  
    // WiFi Name
   // Number in Array
    String currentSSID = WiFi.SSID(networkIndex);
    int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());
    
    if (meshNameIndex >= 0) {
      uint64_t targetNodeID = stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));
      //Serial.println("TargetNodeID = " + (targetNodeID));
      Serial.println(meshInstance.getNodeID());
      //if (targetNodeID < stringToUint64(meshInstance.getNodeID())) {
        ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(networkIndex));
      //}
    }
    // targetNodeId = c9fe88
    //
 }
 Serial.println();
}

void setup() {
  // Prevents the flash memory from being worn out, see: https://github.com/esp8266/Arduino/issues/1054 .
  // This will however delay node WiFi start-up by about 700 ms. The delay is 900 ms if we otherwise would have stored the WiFi network we want to connect to.
  WiFi.persistent(false);

  Serial.begin(115200);
  delay(50); // Wait for Serial.

  //yield(); // Use this if you don't want to wait for Serial.

  // The WiFi.disconnect() ensures that the WiFi is working correctly. If this is not done before receiving WiFi connections,
  // those WiFi connections will take a long time to make or sometimes will not work at all.
  WiFi.disconnect();

  Serial.println();
  Serial.println(F("Setting up mesh node..."));

  /* Initialise the mesh node */
  meshNode.begin();
  meshNode.activateAP(); // Each AP requires a separate server port.
  meshNode.setNodeID("SENSOR1");
  meshNode.setStaticIP(IPAddress(192, 168, 4, 13)); // Activate static IP mode to speed up connection times.
}

int32_t timeOfLastScan = -10000;
void send_message(String message)
{
  String request = String(F("Temperature = ")) + message + 
                       String(requestNumber) + 
                       String(F(" from ")) + 
                       meshNode.getNodeID()+ 
                       String(F("."));
                       
      meshNode.attemptTransmission(request, false);
  
      // One way to check how attemptTransmission worked out
      if (ESP8266WiFiMesh::latestTransmissionSuccessful()) {
        Serial.println(F("Transmission successful."));
      }
       if (ESP8266WiFiMesh::latestTransmissionOutcomes.empty()) {
        Serial.println(F("No mesh AP found."));
      }
      Serial.println();
 }
// meshNode.connectToNode()
void loop() {
  // Give other nodes some time to connect between data transfers.
  // Scan for networks with two second intervals when not already connected.
  if (millis() - timeOfLastScan > 3000 || (WiFi.status() != WL_CONNECTED && millis() - timeOfLastScan > 2000)) 
  {   
      timeOfLastScan = millis();
      send_message(String(random(0,100)));
      
  }  else {
     meshNode.acceptRequest();
  }
}
