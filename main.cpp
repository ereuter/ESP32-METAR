// This program retrieves METAR data from aviationweather.gov and displays it on
// the OLED display on the Heltec ESP32 WiFi board.

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <heltec.h>  //https://github.com/HelTecAutomation/Heltec_ESP32

//Enter SSID and password for your network below
const char* ssid = "SSID";
const char* password = "Password";

// Change the station ID at the end for your airport.
String serverName = "https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=3&mostRecent=true&stationString=KPSM";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 59000;  //this causes an immediate update at startup
unsigned long timerDelay = 60000; //time between updates (60000 ms = 1 min)


//This function looks for xml tags and returns the data between them.
String parse(String input, String value){
  String start = "<"+value+">";
  int startIndex = input.indexOf(start)+start.length();
  String end = "</" + value + ">";
  int endIndex = input.indexOf(end);
  return input.substring(startIndex, endIndex);
}



void setup() {
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->setFont(ArialMT_Plain_10);
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0, "Connecting to \n" + String(ssid));
  Heltec.display->display();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Heltec.display->drawString(0, 32, "Connected at " + WiFi.localIP().toString());
  Heltec.display->display();
  delay(1000);

}

void loop() {
  //Send an HTTP POST request every minute
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName;

      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());

      // Send HTTP GET request
      int httpResponseCode = http.GET();

      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        String input = http.getString();

        // Parse whatever data fields you need
        String observation_time = parse(input, "observation_time");
        String dewpoint_c = parse(input, "dewpoint_c");
        String temp_c = parse(input, "temp_c");
        String wind_dir_degrees = parse(input, "wind_dir_degrees");
        String wind_speed_kt = parse(input, "wind_speed_kt");
        String wind_gust_kt = parse(input, "wind_gust_kt");
        String flight_category = parse(input, "flight_category");
        float altim_in_hg = parse(input, "altim_in_hg").toFloat();
        String visibility_statute_mi = parse(input, "visibility_statute_mi");
        altim_in_hg = round(altim_in_hg*1000)/1000;
        String altim = String(altim_in_hg);

        // Send data to the display
        Heltec.display->clear();
        Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
        Heltec.display->drawString(0, 0, "KPSM METAR " + observation_time.substring(11));
        Heltec.display->drawString(0, 12, "Temp: " +String(temp_c)+"  Dewpt: "+String(dewpoint_c));

        // index will return -1 if the string doesn't exist.  "gust" will only exist if
        // gusts are reported.  This is untested.
        if (input.indexOf("gust")!=-1){
          Heltec.display->drawString(0, 24, "Wind: " + String(wind_dir_degrees)+" at "+String(wind_speed_kt)+"G"+wind_gust_kt+ " kt");
        }
        else Heltec.display->drawString(0, 24, "Wind: " + String(wind_dir_degrees)+" at "+String(wind_speed_kt)+" kt");
        Heltec.display->drawString(0, 36, "Visibility: "+ visibility_statute_mi + " mi");
        Heltec.display->drawString(0, 48, "Altimeter: " + String(altim_in_hg) +"      "+flight_category);

        // Redraw display
        Heltec.display->display();
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}
