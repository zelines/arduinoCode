#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include "dht11.h"

#define REQ_BUF_SZ   60
#define DHT11PIN 5
#define SPIsd 4

#define RELAY1 22
#define RELAY2 23
#define RELAY2 24
#define RELAY2 25

long int cnt=120000;
EthernetClient client;
int relays[4]={22,23,24,25};
float inputsState[2]={0,0};

dht11 DHT11; 
int vetor[4] = {0, 0, 0, 0 };//estado dos reles
float maxMin[4]={100,-100,100,-100};
int incomingByte = 0;

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 0, 0, 3); // IP address, may need to change depending on network
EthernetServer server(80);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;

//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
	return 1.8 * celsius + 32;
}

// fast integer version with rounding
//int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}


//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
	return celsius + 273.15;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
	// (1) Saturation Vapor Pressure = ESGG(T)
	double RATIO = 373.15 / (273.15 + celsius);
	double RHS = -7.90298 * (RATIO - 1);
	RHS += 5.02808 * log10(RATIO);
	RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
	RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
	RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
	double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558 - T);
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
	double a = 17.271;
	double b = 237.7;
	double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
	double Td = (b * temp) / (a - temp);
	return Td;
}






void trataDht11(){
  //Serial.println("\n");

                int chk = DHT11.read(DHT11PIN);

               // Serial.print("Read sensor: ");
                switch (chk)
              {
    case DHTLIB_OK: 
		//Serial.println("OK"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		//Serial.println("Checksum error"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		//Serial.println("Time out error"); 
		break;
    default: 
		//Serial.println("Unknown error"); 
		break;
  }
  
  inputsState[0]=(float)DHT11.temperature;
  inputsState[1]=(float)DHT11.humidity;
  // temperatura
    if(inputsState[0]<maxMin[0]){
    maxMin[0]=inputsState[0];
    }
    if(inputsState[0]>maxMin[1]){
    maxMin[1]=inputsState[0];
    }
    //humidade max min
    if(inputsState[1]<maxMin[2]){
    maxMin[2]=inputsState[1];
    }
    if(inputsState[1]>maxMin[3]){
    maxMin[3]=inputsState[1];
    }
  //ToFile();
  /*
  Serial.print("Humidity (%): ");
  Serial.println((float)DHT11.humidity, 2);

  Serial.print("Temperature (°C): ");
  Serial.println((float)DHT11.temperature, 2);

  Serial.print("Dew Point (°C): ");
  Serial.println(dewPoint(DHT11.temperature, DHT11.humidity));

  Serial.print("Temeratua minima: ");
  Serial.println(maxMin[0]);
  
  Serial.print("Temperatura maxima:(°C): ");
  Serial.println(maxMin[1]);
  
  Serial.print("Humidade Minima: ");
  Serial.println(maxMin[2]);
  
  Serial.print("Humidade Maxima: ");
  Serial.println(maxMin[3]);
  
*/
}

void sendPdf(EthernetClient cl){
  //cl.println("HTTP/1.1 200 OK");
 cl.println("Content-Type: text/plain");
cl.println("Content-Disposition: attachment; filename=logfile.txt");
cl.println("Connection: close");client.println("Connection: keep-alive");
 cl.println(); 
   File webFile = SD.open("logfile.txt");
  if (webFile) {
    while(webFile.available()) {
      Serial.println("Uploding....");
      cl.write(webFile.read());
    }
    
  } else {
    Serial.print("UPLOAD ERROR :Erro SD CARD: ");
   // Serial.println(filename);
  }
 webFile.close();
 delay(1);
    cl.stop(); 
} 

void trataR(){
  
  int i=0; 
  
  for(i=0;i<4;i++){
  
 if(vetor[i]==1){
                      
                      digitalWrite(relays[i],HIGH);
                      delay(100);
                      Serial.println("Relay ligado ");
                    } 
                    
else 
    if(vetor[i]==0){
                      
                      digitalWrite(relays[i],LOW);
                      delay(100);
                      Serial.println("Relay desligado ");

                      
                    }
  }
}

File logFile;

void SaveToFile(){
  digitalWrite(12,HIGH);
  logFile=SD.open("logfile.txt", FILE_WRITE); 
  
  if(logFile){
  Serial.print("save Status ...");
  
  
  logFile.print( inputsState[0]);
  logFile.print(";");
  logFile.print( inputsState[1]);
  logFile.print(";");
  logFile.print(vetor[0]);
  logFile.print(";");
  logFile.print(vetor[1]);
  logFile.print(";");
  logFile.print(vetor[2]);
  logFile.print(";");
  logFile.print(vetor[3]);
  logFile.print(";");
  logFile.print(maxMin[0]);
  logFile.print(";");
  logFile.print(maxMin[1]);
  logFile.print(";");
  logFile.print(maxMin[2]);
  logFile.print(";");
  logFile.print(maxMin[3]);
  logFile.print(";");
  logFile.println();
  
  logFile.close();
   Serial.print("Saved :)");
  
  }else 
    Serial.println("error opening test.txt");
 
 digitalWrite(12,LOW);
}

//////////////////////////////Setup
void setup()
{
  pinMode(RELAY1,OUTPUT);
  digitalWrite(RELAY1,HIGH);  
  pinMode(SPIsd,OUTPUT);
  digitalWrite(SPIsd,HIGH);
  pinMode(12,OUTPUT);
  digitalWrite(12,HIGH);
  Serial.begin(9600);
  Serial.println("DHT11 TEST PROGRAM ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();
  
  ;
  
   // initialize SD card AND ETHERNET
    Serial.println("Initializing SD card...");
    if (!SD.begin(SPIsd)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file."); 
    Serial.println("loadig server ...");
    
    Ethernet.begin(mac, ip);  // initialize Ethernet device
    server.begin();           // start to listen for clients
    Serial.println("Server online... : ) Local  IP: ");
    Serial.print(Ethernet.localIP());
}


void loop()
{
  cnt++;
  if (cnt>=150000){
  trataDht11();
  SaveToFile();
  cnt=0;

  }
 
  if (Serial.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial.read();

                // say what you got:
                //Serial.print("I received: ");
                //Serial.println(incomingByte, DEC);
                
                //ANALOG ;TEMPERATURE READ
                /*
                 int sensorValue = analogRead(A0);
                 float voltage= sensorValue * (5.0 / 1023.0);
                 */
                 
                  if(incomingByte==49){
                    trataDht11();
                   // SaveToFile();
                    //Serial.println(voltage);
                  }
                  if(incomingByte==50){
                   trataR();
                     SD.remove("logfile.txt");
                  } 
                  
                  if(incomingByte==51){
                   
                     SD.remove("logfile.txt");
                  }
                  if(incomingByte==52){
                    //trata
                    Serial.print(inputsState[0]);Serial.print("\t");Serial.print(inputsState[1]);
                    Serial.println("");
                  }
  }
  
 
  client = server.available();  // try to get client

    if (client) { 
         trataDht11();//update sensor data
         // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        
                        SetRelays();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection:keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    if (StrContains(HTTP_req, "\logfile.txt")) {
                        // send rest of HTTP header
                        //client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type:text/plain");
                    client.println("Content-Disposition:attachment; filename=logfile.txt");
                    client.println("Connection:close");
                   client.println(); 
                     File webFile = SD.open("logfile.txt");
                  if (webFile) {
                    while(webFile.available()) {
                    Serial.println("Uploding....");
                    client.write(webFile.read());
                    }
    
                } else {
              Serial.print("UPLOAD ERROR :Erro SD CARD: ");
             // Serial.println(filename);
  }
 webFile.close();

 delay(1);
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(5);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
//delay(1000);
}
void SetRelays(void)
{
    // Relay 1 
    if (StrContains(HTTP_req, "RELAY1=1")) {
        vetor[0] = 1;  
        trataR();
    }
    else if (StrContains(HTTP_req, "RELAY1=0")) {
        vetor[0] = 0;  // save LED state
        trataR();
    }
    // LED 2 (pin 7)
    if (StrContains(HTTP_req, "RELAY2=1")) {
      Serial.println("entrooooooo\n");
        vetor[1] = 1;  // save LED state
        trataR();
    }
    else if (StrContains(HTTP_req, "RELAY2=0")) {
        vetor[1] = 0;  // save LED state
        trataR();
    }
    // LED 3 (pin 8)
    if (StrContains(HTTP_req, "RELAY3=1")) {
        vetor[2] = 1;  // save LED state
        trataR();
    }
    else if (StrContains(HTTP_req, "RELAY3=0")) {
        vetor[2] = 0;  // save LED state
        trataR();
    }
    // LED 4 (pin 9)
    if (StrContains(HTTP_req, "RELAY4=1")) {
        vetor[3] = 1;  // save LED state
        trataR();
    }
    else if (StrContains(HTTP_req, "RELAY4=0")) {
        vetor[3] = 0;  // save LED state
        trataR();
    } 
    else if(StrContains(HTTP_req, "PDFDOWN")){
      Serial.println("Uploding");
      sendPdf(client);
      Serial.println("Uploded");
    }
}


void XML_response(EthernetClient cl)
{
    
    int count;                 // used by 'for' loops
    
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    // read analog inputs
      cl.print("<analog>");
        cl.print(inputsState[0]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(inputsState[1]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(maxMin[0]);cl.print("/");cl.print(maxMin[1]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(maxMin[2]);cl.print("/");cl.print(maxMin[3]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(vetor[0]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(vetor[1]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(vetor[2]);
      cl.println("</analog>");
      cl.print("<analog>");
        cl.print(vetor[3]);
      cl.println("</analog>");
    cl.print("</inputs>");
}
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }
    return 0;
}
//
// END OF FILE
//
