#include "onvif.h"
#include <SPI.h>
#include <Ethernet.h>


byte *nonce;
char *createTime;
char *username;
char *password;
char *profile;
unsigned long beginMicros, endMicros;
extern struct soap_Header soap_Header;
extern struct soap_Body soap_Body;
// static char *soap_HeaderSecurity;

byte mac[] = {  0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
unsigned long byteCount = 0;
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.google.com";

IPAddress server(192, 168, 1, 70);
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
NTPClient timeClient(Udp, timeServer);

// bool printWebData = true;  // set to false for better speed measurement

byte *getNonce();

void setup() {
    Serial.begin(115200);
    // You can use Ethernet.init(pin) to configure the CS pin
    // Ethernet.init(10);  // Most Arduino shields
    // disable SD SPI
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);
    Serial.println(F("Starting ethernet..."));
    if (!Ethernet.begin(mac)) Serial.println(F("failed"));
    else {
        Serial.println(Ethernet.localIP());
        Serial.println(Ethernet.gatewayIP());
    }
    Udp.begin(localPort);

    // createTime = "2018-11-07T18:15:45.000Z";
    username = "admin";
    password = "Supervisor";
    profile = "PROFILE_000";
    nonce = getNonce();

    // allocating memory for ONVIF command text and putting it in Body structre
    // probably should pack all commands in some kind of dictionary
    // soap_Body.soap_Command = (char *)malloc(onvif_command_GetSystemDateAndTime);
    // strcpy(soap_Body.soap_Command, onvif_command_GetSystemDateAndTime);
}

void loop() {
    if (!createTime) {
        timeClient.update();
//        createTime = getISOFormattedTime(&timeClient);
          createTime = "2022-02-11T22:32:40.593Z";
         Serial.print("Time: ");
         Serial.println(createTime);
        // if(authorization_needed) {
        soap_Header.soap_HeaderSecurity = (char*)malloc(2); //not sure how it works again; attempt to allocate proper amount of memory makes arduino hang
        strcpy(soap_Header.soap_HeaderSecurity, calculateHeaderSecurity(username, password, createTime, nonce));
    }
    Serial.println("bleep");
    onvifGetHostname(server);
    delay(2000);
    Serial.println("bloop");
//    onvifGetSystemDateAndTime(server, &soap_Header, &soap_Body);
    onvifContinuousMove(server, &soap_Header, &soap_Body, profile, "1", "0");
//    Serial.println("1");
//    delay(1000);
//    onvifContinuousMove(server, &soap_Header, &soap_Body, profile, "-0.5", "0");
//    Serial.println("2");
//    delay(1000);
//    onvifContinuousMove(server, &soap_Header, &soap_Body, profile, "0", "0.5");      // вызывается функция отправки байта
//    Serial.println("3");
//    delay(1000);
//    onvifContinuousMove(server, &soap_Header, &soap_Body, profile, "0", "-0.5");      // вызывается функция отправки байта
//    Serial.print("4");
//    delay(1000);
//    Serial.print("call of PTZ stop at ");
//    Serial.println(millis());
//    onvifContinuousMove(server, &soap_Header, &soap_Body, profile, "0", "0");
//    Serial.print("PTZ stopped at ");
//    Serial.println(millis());
    while(true) delay(1);
}

// void printHash(uint8_t *hash) {
//     int i;
//     for (i = 0; i < 20; i++) {
//         Serial.print("0123456789abcdef"[hash[i] >> 4]);
//         Serial.print("0123456789abcdef"[hash[i] & 0xf]);
//     }
//     Serial.println();
// }
//
// void printNonce(byte nonce[]) {
//     int i;
//     for (i = 0; i < 20; i++) {
//         Serial.print(nonce[i], HEX);
//     }
//     Serial.println();
// }

byte *getNonce() {
    byte random_num;
    static byte random_arr[20];
    int i = 0;
    for (i = 0; i < 20; i++) {
        random_num = random(255);
        random_arr[i] = random_num;
    }
    return random_arr;
}
