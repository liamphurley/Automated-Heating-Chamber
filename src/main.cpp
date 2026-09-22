#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>

#define ONE_WIRE_BUS 2  // GPIO connected to the DS18B20 data pin

#define heatPin 5

#define fanPin 19

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int goalTemp = 35;

float lastHeatUpdate =0;
int lastRecordUpdate=0;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET -1

#define RECORDS 5000
int recordCount =0;
struct TemperatureRecord {
    unsigned long time;
    float temperature;
    bool heaterOn;
};
TemperatureRecord recArray[RECORDS];

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);
const char* ssid = "";
const char* password = "";

String graph ="";

String setGraph(TemperatureRecord arr[]);

void setup() {
    Serial.begin(115200);
    sensors.begin();

    //Pin Setup
    pinMode(heatPin, OUTPUT);
    pinMode(fanPin, OUTPUT);

    // Start the OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      while (true); // Stop if display isn't found
    }

    // Display text OLED
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 5);



    //Server
      // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());

    server.on("/", []() {
    String webpage = "<html><head><style>body { background-color: #1e1e2e; color: #f5f5f5; font-family: sans-serif; padding: 20px; }</style></head><body>Current Temp: " + String(sensors.getTempCByIndex(0));
    webpage += "<br><br>Goal Temp: " + String(goalTemp) + "<br><br> Goal Temp Input";

    webpage += "<form action='/set'>";
    webpage += "<input name='temp' type='number'>";
    webpage += "<input type='submit' value='Set'>";
    webpage += "</form><br><pre>" + graph + "</pre>";

    server.send(200, "text/html", webpage);
    });

    server.on("/set", []() {
    goalTemp = server.arg("temp").toInt();
    server.sendHeader("Location", "/");
    server.send(303);
    });

    server.begin();

    display.clearDisplay();

  }

void loop() {
    
    sensors.requestTemperatures();

    float tempC = sensors.getTempCByIndex(0);

    if (tempC<goalTemp-0.5)
    {
      if(millis() - lastHeatUpdate >= 5000)
      {
        digitalWrite(heatPin, HIGH);
        lastHeatUpdate=millis();
        digitalWrite(fanPin, HIGH);
      }
    } else if(goalTemp<tempC)
    {
      if(millis() - lastHeatUpdate >= 500)
      {
        digitalWrite(heatPin, LOW);
        lastHeatUpdate=millis();
        digitalWrite(fanPin, HIGH);
      }
    }

    if ((recordCount <RECORDS)&&(millis()-lastRecordUpdate)>=500)
    {
      recArray[recordCount].time = millis();
      recArray[recordCount].temperature = tempC;
      recArray[recordCount].heaterOn = digitalRead(heatPin);

      recordCount++;
    }


  
    display.clearDisplay();
    display.setCursor(10,5);
    display.print("Current Temp");
    display.setCursor(10,15);
    display.print(tempC);
    display.setCursor(10,25);
    display.print("Goal Temp");
    display.setCursor(10,35);
    display.print(goalTemp);
    display.display();
 
    server.handleClient();
}


String setGraph(TemperatureRecord arr[]) {

  const int columns = 102;
  int rows = 20+2;

  String graph = " ";
  String finalXaxis ="";
  String chunkXaxis ="";

  int plotable[columns - 2];
  int plotSum=0;
  int targetRow;

  if(recordCount>(columns-2))
  {
    for(int i=0 ; i<(columns-2); ++i)
      {for (int j=(i*(recordCount/(columns-2))) ; j<(recordCount/(columns-2))+(i*(recordCount/(columns-2))); ++j)
        {
          plotSum += arr[j].temperature;
        }
      plotable[i]=plotSum/(recordCount/(columns-2));
      plotSum=0;
    }
  }

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {

      if(j!=80&&j!=81)

      if(!(recordCount>(columns-2)))
        {targetRow = 20 - ((int)round(arr[j].temperature - (goalTemp - ((rows-2)/2)) - 1));
        }else{
          targetRow = 20 - ((int)round(plotable[j] - (goalTemp - ((rows-2)/2)) - 1));
        }

      if (j == columns - 1) {
        graph += '\n';  // Add newline at end of row
        graph += "|";
        if(i < rows-1)
        {
          graph += goalTemp+((rows-2)/2)-i;
        }
      } else if(i < rows-1){
        if(targetRow==i)
        {
          graph+="#";
        }else{
          graph+=" ";
        }
      }
    }
  }

  for(int i=0 ; i<columns; ++i)
  {
    graph += "_";
  }

  int Xaxis[columns - 2];
  int lastDigit = 1000000;
  int characterCount =6;

  if(recordCount>(columns-2))
  {
    for(int i=0 ; i<(columns-2); ++i)
      {for (int j=(i*(recordCount/(columns-2))) ; j<(recordCount/(columns-2))+(i*(recordCount/(columns-2))); ++j)
        {
          plotSum += arr[j].time;
        }
      Xaxis[i]=plotSum/(recordCount/(columns-2));
      plotSum=0;
    }
    for(int i=-5 ; i<(columns-2); ++i)
    {
      if(i>2)
      {
        if(i%5==0)
        {

            if(lastDigit == Xaxis[i]/60000)
            {
              finalXaxis += "-----";
            }else{
              finalXaxis += Xaxis[i]/60000;
              
              if(Xaxis[i]/60000<10)
              {
                finalXaxis += "----";
              }else if(Xaxis[i]/60000<100)
              {
                finalXaxis += "---";
              }else if(Xaxis[i]/60000<1000)
              {
                finalXaxis += "--";
              }
              lastDigit = Xaxis[i]/60000;
            }
        }else{
          //finalXaxis += ' ';
        }
      }else{
        finalXaxis += '-';
      }
    }
  }
  

  graph += '\n';
  graph += finalXaxis;

  return graph;
}