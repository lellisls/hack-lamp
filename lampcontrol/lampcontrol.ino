// Projeto Curto Circuito – Blynk ESP32
#include <BLEDevice.h> //Header file for BLE
#include <WiFi.h>

#include "WifiAuth.h"

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String lightState = "off";

// Assign output variables to GPIO pins
const int light = 2;

const char *lightOn = "19E5AA";

const char *MAC_ADDRESS = "F4:5E:AB:9D:A1:1A";
const char *MAIN_SERVICE_UUID = "00005251-0000-1000-8000-00805f9b34fb";
const char *LIGHT_INTENSITY_UUID = "00002501-0000-1000-8000-00805f9b34fb";
const char *AUTH_UUID = "00002506-0000-1000-8000-00805f9b34fb";

// Set your Static IP address
IPAddress local_IP(192, 168, 15, 123);
// Set your Gateway IP address
IPAddress gateway(192, 168, 15, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

static BLEUUID serviceUUID(MAIN_SERVICE_UUID); //Service UUID of fitnessband obtained through nRF connect application
static BLEUUID charUUID(LIGHT_INTENSITY_UUID); //Characteristic  UUID of fitnessband obtained through nRF connect application
String My_BLE_Address = MAC_ADDRESS;           //Hardware Bluetooth MAC of my fitnessband, will vary for every band obtained through nRF connect application

static BLERemoteCharacteristic *pRemoteCharacteristic;

//Name the scanning device as pBLEScan
BLEScan *pBLEScan;
BLEScanResults foundDevices;

static BLEAddress *Server_BLE_Address;
String Scaned_BLE_Address;

boolean paired = false; //boolean variable to togge light

bool connectToserver(BLEAddress pAddress)
{

    BLEClient *pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    // Connect to the BLE Server.
    pClient->connect(pAddress);
    Serial.println(" - Connected to fitnessband");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService != nullptr)
    {
        Serial.println(" - Found our service");
        return true;
    }
    else
        return false;

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic != nullptr)
        Serial.println(" - Found our characteristic");

    return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
        Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());

        Scaned_BLE_Address = Server_BLE_Address->toString().c_str();
    }
};

void setup()
{
    Serial.begin(115200);
    // Initialize the output variables as outputs
    pinMode(light, OUTPUT);
    // Set outputs to LOW
    digitalWrite(light, LOW);

    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
        Serial.println("STA Failed to configure");
    }

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();

    Serial.println("ESP32 BLE Server program"); //Intro message

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();                                           //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); //Call the class that is defined above
    pBLEScan->setActiveScan(true);                                             //active scan uses more power, but get results faster
}

void writeBluetoothValue(const char * lightString)
{
    Serial.print("Setting output to ");
    Serial.println(lightString);

    while (foundDevices.getCount() >= 1)
    {
        if (Scaned_BLE_Address == My_BLE_Address && paired == false)
        {
            Serial.println("Found Device :-)... connecting to Server as client");
            if (connectToserver(*Server_BLE_Address))
            {
                paired = true;
                Serial.println("********************LED turned ON************************");
                digitalWrite(13, HIGH);
                break;
            }
            else
            {
                Serial.println("Pairing failed");
                break;
            }
        }

        if (Scaned_BLE_Address == My_BLE_Address && paired == true)
        {
            Serial.println("Our device went out of range");
            paired = false;
            Serial.println("********************LED OOOFFFFF************************");
            digitalWrite(13, LOW);
            ESP.restart();
            break;
        }
        else
        {
            Serial.println("We have some other BLe device in range");
            break;
        }
    }
}

void loop()
{

    foundDevices = pBLEScan->start(3); //Scan for 3 seconds to find the Fitness band

    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    {                                  // If a new client connects,
        Serial.println("New Client."); // print a message out in the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        while (client.connected())
        { // loop while the client's connected
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                header += c;
                if (c == '\n')
                { // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        // turns the GPIOs on and off
                        if (header.indexOf("GET /light/on") >= 0)
                        {
                            Serial.println("Light turned on");
                            lightState = "on";
                            // digitalWrite(light, HIGH);
                        }
                        else if (header.indexOf("GET /light/off") >= 0)
                        {
                            Serial.println("Light turned off");
                            lightState = "off";
                            // digitalWrite(light, LOW);
                        }

                        writeBluetoothValue("00FFAA");

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        // CSS to style the on/off buttons
                        // Feel free to change the background-color and font-size attributes to fit your preferences
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");

                        // Web Page Heading
                        client.println("<body><h1>ESP32 Web Server</h1>");

                        // Display current state, and ON/OFF buttons for the light
                        client.println("<p>Light - State " + lightState + "</p>");
                        // If the lightState is off, it displays the ON button
                        if (lightState == "off")
                        {
                            client.println("<p><a href=\"/light/on\"><button class=\"button\">ON</button></a></p>");
                        }
                        else
                        {
                            client.println("<p><a href=\"/light/off\"><button class=\"button button2\">OFF</button></a></p>");
                        }
                        client.println("</body></html>");

                        // The HTTP response ends with another blank line
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}