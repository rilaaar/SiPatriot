
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ESP8266WiFiMulti.h> 

ESP8266WiFiMulti wifiMulti;

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#define API_KEY "AIzaSyAP3WeCaxfiHbASklyc6T3ouOQ2tGOVilA"
#define DATABASE_URL "database-sipatriot-default-rtdb.firebaseio.com"
#define USER_EMAIL "winrarelectro2023@gmail.com"
#define USER_PASSWORD "rilaamirramlan"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
unsigned long count = 0;
String wifi_stat = "Disconnected";


const long utcOffsetInSeconds = 25200; //GTM is +7 = 7*60*60 = 25200
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String updateTime;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,16,2);
//LiquidCrystal_I2C lcd(0x27,16,2);

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2 // D4

// Setup a oneWire instance to communicate with any OneWire device
  OneWire oneWire(ONE_WIRE_BUS);  
// Pass oneWire reference to DallasTemperature library
  DallasTemperature sensors(&oneWire);

const int relay = 0; //pin D3 
const int tombolatas1 = 12; //pin D6 -LOW (Gnd)
const int tombolatas2 = 15; //pin D8 -HIGH (3V)
const int tombolbawah1 = 14; //pin D5 -LOW (Gnd)
const int tombolbawah2 = 13; //pin D7 -LOW (Gnd)

float ds18b20=0;

int batasAtas2;
int batasBawah2;

String batasBawah = "0";
String batasAtas = "0";

String heater;

char buf[100];
char buf1[100];

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();

  return now;
  }

String twoDigits(int digits) {
  if (digits < 10) {
    String formatted = "0";
    formatted += String(digits);
    return formatted;
  }
  return String(digits);
}


void setup() {
  Serial.begin(9600);
  lcd.begin ();

  timeClient.begin();

  sensors.begin();
  pinMode(relay,OUTPUT);
  pinMode(tombolatas1,INPUT_PULLUP);
  pinMode(tombolbawah1,INPUT_PULLUP);
  pinMode(tombolatas2,INPUT_PULLUP);  
  pinMode(tombolbawah2,INPUT_PULLUP);

  lcd.backlight();
  lcd.setBacklight(HIGH);
  lcd.setCursor(3, 0);
  lcd.print("WELCOME TO");
  lcd.setCursor(3.5, 1);
  lcd.print("SI PATRIOT");
  delay(3500);
  lcd.clear();
  lcd.noCursor();

  wifiMulti.addAP("WIFI GOA", "tuhanyanganeh");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("Syadidannn.", "112223333");
  wifiMulti.addAP("Aspire 5", "asdfg123");
  wifiMulti.addAP("kijang-1", "asdf1234");
                 
  Serial.println(); 
  Serial.println("**************");                                 
  Serial.print("Connecting to ");
  Serial.print(WiFi.SSID());

  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(500);
    Serial.print('.');
    wifi_stat = "Connected";
  }
 
  Serial.print(wifi_stat);
  Serial.println();
  Serial.print("Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());     

  lcd.setCursor(1,0);
  lcd.print("WIFI CONNECTING TO : ");
  lcd.setCursor(0,1);
  lcd.println(WiFi.SSID());
  delay(2000);
  lcd.clear();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

}

void loop() {
  //kode DS18B20
  sensors.requestTemperatures(); 
  ds18b20=sensors.getTempCByIndex(0);

  if (isnan(ds18b20)) {         // Check if any reads failed.
    Serial.println(("Failed to read from DS18B20 sensor!"));
    lcd.setCursor(3,0);
    lcd.print("Error!");
    lcd.setCursor(3,1);
    lcd.print("Error!");
    return;
   }

   //print the temperature in Celsius
  Serial.print("Suhu: ");
  Serial.print(ds18b20);
  Serial.print(" °C ");
  Serial.println();


  // Convert epoch time to formatted string
  String plusChar = "+";
  String formattedTime = daysOfTheWeek[timeClient.getDay()]+ plusChar +
                         twoDigits(hour(getTime())) + "." +
                         twoDigits(minute(getTime())) + "." +
                         twoDigits(second(getTime()));



  if (Firebase.ready() && (millis() - sendDataPrevMillis > 500 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    
    gcvt(ds18b20, 3, buf);
    batasBawah = Firebase.getString(fbdo, F("/batasbawah")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str();
    batasAtas = Firebase.getString(fbdo, F("/batasatas")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str();
    
    readButton();
    

    Serial.printf("Set timestamp... %s\n", Firebase.setInt(fbdo, F("/timestamp"), getTime()) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get timestamp... %s\n", Firebase.getInt(fbdo, F("/timestamp")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());

    Serial.printf("Set updateTime... %s\n", Firebase.setString(fbdo, F("/updateTime"), formattedTime) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get updateTime... %s\n", Firebase.getString(fbdo, F("/updateTime")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());

    readButton();
    
    Serial.printf("Set Temperature... %s\n", Firebase.setString(fbdo, F("/Temperature"), buf) ? "ok" : fbdo.errorReason().c_str());
    Serial.printf("Get Temperature... %s\n", Firebase.getString(fbdo, F("/Temperature")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());

    readButton();
    
    Serial.printf("Get batas atas... %s\n", Firebase.getString(fbdo, F("/batasatas")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());

    readButton();
    
    Serial.printf("Get batas bawah... %s\n", Firebase.getString(fbdo, F("/batasbawah")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());

    readButton();
    
    Serial.printf("Get heater status... %s\n", Firebase.getString(fbdo, F("/HeaterStatus")) ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());  
    heater = Firebase.getString(fbdo, F("/HeaterStatus"))? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str();

    readButton();
    
    Serial.println();
    count++;
    }


  if (heater == "1") {
    if (ds18b20 <= batasBawah.toInt()){
      digitalWrite(relay ,HIGH);
      Serial.printf("\nHeater ON\n");
       }
    else if (ds18b20 >= batasAtas.toInt()){
    digitalWrite (relay,LOW);
    Serial.printf("\nHeater OFF\n");
       }
   }
   else {
    digitalWrite(relay, LOW);
    Serial.printf("\nHeater OFF\n");
   }
  
    readButton();
}

void readButton (){
  int statustombolatas1 = digitalRead(tombolatas1);
  int statustombolbawah1 = digitalRead(tombolbawah1);
  int statustombolatas2 = digitalRead(tombolatas2);
  int statustombolbawah2 = digitalRead(tombolbawah2);

//================BATAS ATAS=====================
  batasAtas2 = batasAtas.toInt();
  if (statustombolatas1 == LOW){
    Serial.printf("\nstatustombolatas1 ditekan\n");
    batasAtas2++;
    gcvt(batasAtas2, 3, buf1);
    Serial.printf("batas atas 2 : %d", batasAtas2);
    Firebase.setString(fbdo, F("/batasatas"), buf1) ? String(buf1) : String(fbdo.errorReason().c_str());
  }

  if (statustombolbawah1 == LOW){
     Serial.printf("\nstatustombolbawah1 ditekan\n");
     batasAtas2--;
     gcvt(batasAtas2, 3, buf1);
     Serial.printf("batas atas 2 : %d", batasAtas2);
     Firebase.setString(fbdo, F("/batasatas"), buf1) ? String(buf1) : String(fbdo.errorReason().c_str());    
  }

//================BATAS BAWAH=====================
  batasBawah2 = batasBawah.toInt();
  if (statustombolatas2 == HIGH){
    Serial.printf("\nstatustombolatas2 ditekan\n");
    batasBawah2 ++;
    gcvt(batasBawah2, 3, buf1);
    Serial.printf("batas bawah 2 : %d", batasBawah2);
    Firebase.setString(fbdo, F("/batasbawah"), buf1) ? String(buf1) : String(fbdo.errorReason().c_str());
  }

  if (statustombolbawah2 == LOW){
    Serial.printf("\nstatustombolbawah2 ditekan\n");
    batasBawah2 --;
    gcvt(batasBawah2, 3, buf1);
    Serial.printf("batas bawah 2 : %d", batasBawah2);
    Firebase.setString(fbdo, F("/batasbawah"), buf1) ? String(buf1) : String(fbdo.errorReason().c_str());
  }

  printLcd();
}

void printLcd(){
  lcd.setCursor(2,0);
  lcd.print("SUHU : ");
  lcd.print(ds18b20);
  lcd.print(" C ");
  lcd.setCursor(3,1);
  lcd.print(batasBawah);
  lcd.print(" C / ");
  lcd.print(batasAtas);
  lcd.print(" C ");  
}