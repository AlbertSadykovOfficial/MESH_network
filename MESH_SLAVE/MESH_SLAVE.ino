#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <TypeConversionFunctions.h>
#include <assert.h>

bool retranslate = false;
String translate = "";

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
  String command = "";
  /* Print out received message */
  if (request == "EXECUTE: START")
  {
    command = "STARTED Successefuly";
    digitalWrite(14, HIGH);
    digitalWrite(12, HIGH);
  }
  else if (request == "EXECUTE: STOP")
  {
    command = "STOPPED Successefuly";
    digitalWrite(14, LOW);
    digitalWrite(12, LOW);
  }
  else
  {
     retranslate = true;
     translate = request + " translated by SLAVE";
     command = "SLAVE:unknown command, I have translated to next node";
  }
  Serial.print(F("DATA_TO_ME: "));
  Serial.println(request);
  //String command = meshInstance.getMessage();
  Serial.print(F("DATA_FROM_ME: "));
  Serial.println(command);

  /* return a string to send back */
  return command;
}

transmission_status_t manageResponse(const String &response, ESP8266WiFiMesh &meshInstance) 
{
  transmission_status_t statusCode = TS_TRANSMISSION_COMPLETE;

  /* Print out received message */
   String command = meshInstance.getMessage();
  Serial.print(F("DATA_FROM_ME: "));
  Serial.println(command);
  Serial.print(F("DATA_TO_ME: "));
  Serial.println(response);

  // Our last request got a response, so time to create a new request.
  //meshInstance.setMessage(String(F("Hello world request #")) + String(++requestNumber) + String(F(" from "))
    //                      + meshInstance.getMeshName() + meshInstance.getNodeID() + String(F(".")));

  // (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
  return statusCode;
}

// ПОДКЛЮЧЕНИЕ?
void networkFilter(int numberOfNetworks, ESP8266WiFiMesh &meshInstance) {
 for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex) 
 {  
    // WiFi Name
    // Number in Array
    String currentSSID = WiFi.SSID(networkIndex);
    int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());

    if (meshNameIndex >= 0) {
      uint64_t targetNodeID = stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));
      //Serial.println("TargetNodeID = " + (targetNodeID));
      if (targetNodeID < stringToUint64(meshInstance.getNodeID())) {
        ESP8266WiFiMesh::connectionQueue.push_back(NetworkInfo(networkIndex));
      }
    }
 }
 Serial.println();
}

void setup() {
  // Prevents the flash memory from being worn out, see: https://github.com/esp8266/Arduino/issues/1054 .
  // This will however delay node WiFi start-up by about 700 ms. The delay is 900 ms if we otherwise would have stored the WiFi network we want to connect to.
  WiFi.persistent(false);

  Serial.begin(115000);
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
  meshNode.setNodeID("SLAVE1");
  meshNode.setStaticIP(IPAddress(192, 168, 4, 12)); // Activate static IP mode to speed up connection times.

  pinMode(12, OUTPUT);
  pinMode(14, OUTPUT);
}
int32_t timeOfLastScan = -10000;
void send_message(String command)
{
    String request = command;
                     
    meshNode.attemptTransmission(request, false);
    
    timeOfLastScan = millis();
    
    // One way to check how attemptTransmission worked out
    if (ESP8266WiFiMesh::latestTransmissionSuccessful()) {
      Serial.println(F("Transmission successful."));
    }
     if (ESP8266WiFiMesh::latestTransmissionOutcomes.empty()) {
      Serial.println(F("No mesh AP found."));
    }
    Serial.println();

    translate = "";
}

void loop() {
    if (retranslate == true)
    {
        //meshNode.connectToNode(String target_ssid, String message);
        retranslate = false;
        // send_message(translate);
        meshNode.connectToNode(String("MeshNode_SERVER1"), String(translate));
        Serial.println("Translate" + translate);
    }
    meshNode.acceptRequest();
}
