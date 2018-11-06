#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {  0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server(192, 168, 11, 22);
//IPAddress ip(192, 168, 11, 228);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

void setup() {
  Serial.begin(115200);

  // disable SD SPI
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Serial.println(F("Starting ethernet..."));
  if (!Ethernet.begin(mac)) Serial.println(F("failed"));
  else {
//  Ethernet.begin(mac,ip);
    Serial.println(Ethernet.localIP());
    Serial.println(Ethernet.gatewayIP());
  }
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    // Make a HTTP request:
//    client.print("POST /onvif/device_service HTTP/1.1\r\nHost: 192.168.11.102\r\nUser-Agent: curl/7.58.0\r\nAccept: */*\r\nContent-Length: 261\r\nContent-Type: application/x-www-form-urlencoded\r\n");
    client.println("POST /onvif/device_service HTTP/1.1");
//    client.println("POST /onvif/device_service HTTP/1.1\r\nHost: 192.168.11.102\r\nUser-Agent: Arduino/1.0\r\nAccept: */*\r\nContent-Length: 261\r\nContent-Type: application/x-www-form-urlencoded\r\n<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">\r\n<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\r\n<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>\r\n</s:Body></s:Envelope>\r\n");
//    client.println("GET /index.html HTTP/1.1");
    client.println("Host: 192.168.11.22");
//    client.println("Host: 192.168.11.102");
    client.println("User-Agent: Arduino/1.0");
//    client.println("User-Agent: curl/7.58.0");
    client.println("Accept: */*");
//    client.println("Content-Length: 0");
//    client.println("Connection: close");
    client.println("Content-Length: 256");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
//    client.print("<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">");
    client.print("<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\"><s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">");//<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>");//</s:Body>");//</s:Envelope>");
//    client.print("<s:Body xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\r\n<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>\r\n</s:Body>\r\n</s:Envelope>");
//    client.print("<GetSystemDateAndTime xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>");
    client.print("<GetCapabilities xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>");
    client.print("</s:Body>");
    client.print("</s:Envelope>");
//    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  beginMicros = micros();
}

void loop() {
  int len = client.available();
//  if (len > 0 ) Serial.println("\n *** \n Client is available \n *** ");
  if (len > 0) {
    byte buffer[80];
    if (len > 80) len = 80;
    client.read(buffer, len);
//    Serial.println("readed in buffer");
    if (printWebData) {
      Serial.write(buffer, len); // show in the serial monitor (slows some boards)
    }
    byteCount = byteCount + len;
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    endMicros = micros();
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    Serial.print("Received ");
    Serial.print(byteCount);
    Serial.print(" bytes in ");
    float seconds = (float)(endMicros - beginMicros) / 1000000.0;
    Serial.print(seconds, 4);
    float rate = (float)byteCount / seconds / 1000.0;
    Serial.print(", rate = ");
    Serial.print(rate);
    Serial.print(" kbytes/second");
    Serial.println();

    // do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
}
