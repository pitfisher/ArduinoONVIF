#include "onvif.h"
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {  0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server(192, 168, 15, 42);

int sensorPan = A2;     // канал вправо/влево
int sensorTilt = A1;    // канал вверх/вниз
//int sensorFocus = A2;   // канал фокус

int xVal = 0;           // переменная для хранения значения с потенциометра вправо/влево
int yVal = 0;           // переменная для хранения значения с потенциометра вниз/вверх
int focus = 0;          // переменная для хранения значения с потенциометра фокуса камеры

byte *nonce;
char *createTime;
char *username;
char *password;

unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
// bool printWebData = true;  // set to false for better speed measurement

unsigned int localPort = 8888;       // local port to listen for UDP packets

const char timeServer[] = "time.google.com";
// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
NTPClient timeClient(Udp, timeServer);

// static char *soap_HeaderSecurity;
extern struct soap_Header soap_Header;
extern struct soap_Body soap_Body;


void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));
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
    nonce = getNonce();

    // allocating memory for ONVIF command text and putting it in Body structre
    // probably should pack all commands in some kind of dictionary
    // soap_Body.soap_Command = (char *)malloc(onvif_command_GetSystemDateAndTime);
    // strcpy(soap_Body.soap_Command, onvif_command_GetSystemDateAndTime);
}

void loop() {
    if (!createTime) {
        timeClient.update();
        createTime = getISOFormattedTime(&timeClient);
        Serial.print("Time: ");
        Serial.println(createTime);
        // if(authorization_needed) {
        soap_Header.soap_HeaderSecurity = (char*)malloc(2); //not sure how it works again; attempt to allocate proper amount of memory makes arduino hang
        strcpy(soap_Header.soap_HeaderSecurity, calculateHeaderSecurity(username, password, createTime, nonce));
    }
    Serial.println("bloop");
    xVal = analogRead(sensorPan);     // считывание значения потенциометра вправо/влево в соответствующую переменную
    yVal = analogRead(sensorTilt);    // считывание значения потенциометра вниз/вверх в соответствующую переменную
//  focus = analogRead(sensorFocus);  // считывание значения потенциометра фокуса камеры в соответствующую переменную
    Serial.println(yVal);
    Serial.println(xVal);
    if (xVal > 600)
    {
        onvifContinuousMove(server, &soap_Header, &soap_Body, "Profile_1", "0.5", "0");      // вызывается функция отправки байта
        while (xVal > 600)
        {
            xVal = analogRead(sensorPan);
        }
    }
    Serial.println("1");
    if (xVal < 450)
    {
        onvifContinuousMove(server, &soap_Header, &soap_Body, "Profile_1", "-0.5", "0");      // вызывается функция отправки байта
        while (xVal < 450)
        {
            xVal = analogRead(sensorPan);
        }
    }
    Serial.println("2");
    if (yVal > 600)
    {
        onvifContinuousMove(server, &soap_Header, &soap_Body, "Profile_1", "0", "0.5");      // вызывается функция отправки байта
        while (yVal > 600)
        {
            yVal = analogRead(sensorTilt);
        }
    }
    Serial.println("3");
// newSerial.println("3");
    if (yVal < 450)
    {
        onvifContinuousMove(server, &soap_Header, &soap_Body, "Profile_1", "0", "-0.5");      // вызывается функция отправки байта
        while (yVal < 450)
        {
            yVal = analogRead(sensorTilt);
        }
    }
    Serial.print("call of PTZ stop at ");
    Serial.println(millis());
    onvifContinuousMove(server, &soap_Header, &soap_Body, "Profile_1", "0", "0");
    Serial.print("PTZ stopped at ");
    Serial.println(millis());
    // while(1)delay(1);

    // if the server's disconnected, stop the client:
    //  if (!client.connected()) {
    //    endMicros = micros();
    //    Serial.println();
    //    Serial.println("disconnecting.");
    //    client.stop();
    //    Serial.print("Received ");
    //    Serial.print(byteCount);
    //    Serial.print(" bytes in ");
    //    float seconds = (float)(endMicros - beginMicros) / 1000000.0;
    //    Serial.print(seconds, 4);
    //    float rate = (float)byteCount / seconds / 1000.0;
    //    Serial.print(", rate = ");
    //    Serial.print(rate);
    //    Serial.print(" kbytes/second");
    //    Serial.println();
    ////     do nothing forevermore:
    //    while (true) {
    //      delay(1);
    //    }
    //  }
}

void printHash(uint8_t *hash) {
    int i;
    for (i = 0; i < 20; i++) {
        Serial.print("0123456789abcdef"[hash[i] >> 4]);
        Serial.print("0123456789abcdef"[hash[i] & 0xf]);
    }
    Serial.println();
}

void printNonce(byte nonce[]) {
    int i;
    for (i = 0; i < 20; i++) {
        Serial.print(nonce[i], HEX);
    }
    Serial.println();
}

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
