#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#define WIFI_NAME "your wifi name"                    // WiFi Name
#define WIFI_PASS "your wifi password"                // WiFi Password
#define BOTtoken "your telegram bot token"            // your Bot Token (Get from Botfather)
#define CHAT_ID "your telegram chat id"               // your Telegram chat id

#define LDR1  34                                                    // Output Of LDR1 + Resistance Pin
#define LDR2  35                                                    // Output Of LDR2 + Resistance Pin
#define LDR3  32                                                    // Output Of LDR2 + Resistance Pin
#define Switch 27                                                   // connect switch for turn on the motor
#define Photo_Switch 26                                             // Connect Esp32 pin 26 with Esp32-cam module for sending signal and capture photo

#define timeSeconds 10
#define timeSeconds_LDRs 10
#define timeSeconds_LDR1 10
#define timeSeconds_LDR2 10
#define timeSeconds_LDR3 10
#define Time_for_work 180             // work time in seconds
#define Time_for_sleep 120            // sleep time ins seconds

const int motor = 14;
const int wifiled = LED_BUILTIN;

unsigned long now = millis();
unsigned long lastTrigger = 0;
unsigned long lastTrigger_LDRs = 0;
unsigned long lastTrigger_LDR1 = 0;
unsigned long lastTrigger_LDR2 = 0;
unsigned long lastTrigger_LDR3 = 0;

float Level_Percentage = 50.0;

float LDR_Vol1=0.0;
float LDR_Vol2=0.0;
float LDR_Vol3=0.0;

float Intensity1=0.0;
float Intensity2=0.0;
float Intensity3=0.0;
float Intensity_avg=0.0;

bool ledstatus=false;                                                   // Variable For LED Status
bool previous_read=false;
bool SendPhoto=true;
bool Start_Timer=false;

bool send_msg_LDRs=false;
bool send_msg_LDR1=false;
bool send_msg_LDR2=false;
bool send_msg_LDR3=false;

bool Timer_LDRs=false;
bool Timer_LDR1=false;
bool Timer_LDR2=false;
bool Timer_LDR3=false;

String Intensity_status="0";

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
int Motor_On_Count=0;
int Send_Msg_Again=0;
unsigned long lastTimeBotRan;

bool ledState = LOW;

void setupWiFi()
 {
   int count=0;
   Serial.println("Connecting..");
   WiFi.begin(WIFI_NAME, WIFI_PASS);
   while (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Still Connecting....!");
      delay(100);
      count=count+1;
      if(count==15)
        {
          Serial.println(count);
          break;
        }
    }
    if(WiFi.status() == WL_CONNECTED)
      {
        int i=0;
        while(i<5)
          {
            digitalWrite(wifiled, HIGH);
            delay(200);
            digitalWrite(wifiled, LOW);
            delay(200);
            i++;                                                    // 5 Times LED Blink When Connected Successfully With WiFi
          }
        digitalWrite(wifiled, HIGH);
        Serial.println("Connected with wifi Successfully..!");
      }
    else
      {
        Serial.println("RESET ESP32..!!");                          // If Esp32 Not Connected in 15 Cycle Of While Loop Then Reset Again
        ESP.restart();
      } 
  }

void Check_Wifi()
{
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("RESET ESP32..!!");                          // If Esp32 Not Connected in 15 Cycle Of While Loop Then Reset Again
    ESP.restart();
  }
}

void Telegram_bot()
{
  if (millis() > lastTimeBotRan + botRequestDelay)  
    {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while(numNewMessages) 
        {
          Serial.println("got response");
          handleNewMessages(numNewMessages);
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
      lastTimeBotRan = millis();
    }
}

void handleNewMessages(int numNewMessages) 
{
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    
    if (text == "/MOTOR_ON") 
    {
      digitalWrite(motor, LOW);
      bot.sendMessage(chat_id, "MOTOR is ON", "");  
      Serial.println("MOTOR is ON");
      ledstatus=true;
      lastTrigger = millis();
      Start_Timer=true;
    }
    
    else if (text == "/MOTOR_OFF") 
    {
      digitalWrite(motor, HIGH);
      bot.sendMessage(chat_id, "MOTOR is OFF", "");  
      Serial.println("MOTOR is OFF");
      ledstatus=false;
      Motor_On_Count=Motor_On_Count+1;
    }
    else if (text == "/PHOTO") 
    {
      digitalWrite(Photo_Switch,LOW);
      delay(1000);
      digitalWrite(Photo_Switch,HIGH);
      bot.sendMessage(chat_id, "Sending Photo....", "");
      Serial.println("Sending Photo....");
    }
    else if (text == "/STATUS") 
    {
      Intensity_status= "\nSUN intensity is : ";
          Intensity_status += Intensity_avg;
          Intensity_status += "%\n";
          Intensity_status += "\nLDR1 intensity is : ";
          Intensity_status += Intensity1;
          Intensity_status += "%\n";
          Intensity_status += "\nLDR2 intensity is : ";
          Intensity_status += Intensity2;
          Intensity_status += "%\n";
          Intensity_status += "\nLDR3 intensity is : ";
          Intensity_status += Intensity3;
          Intensity_status += "%\n";
      if(ledstatus)
      {
        Intensity_status += "\nMOTOR is ON";
        bot.sendMessage(chat_id, Intensity_status);
        Serial.println(Intensity_status);
      }
      else
      {
        Intensity_status += "\nMOTOR is OFF";
        bot.sendMessage(chat_id, Intensity_status);
        Serial.println(Intensity_status);
      }
    }
   else 
   {
    String welcome = "Welcome, " + from_name + ".\n";
          welcome += "Send your command.\n";
          welcome += "/MOTOR_ON : to switch the Motor ON\n";
          welcome += "/MOTOR_OFF : to switch the Motor OFF\n";
          welcome += "/PHOTO : to know condition of panel\n";
          welcome += "/STATUS : to know status of Motor and Sun intensity\n";
          bot.sendMessage(chat_id, welcome, "");
          Serial.println(welcome);
   }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(wifiled, OUTPUT);
  digitalWrite(wifiled, LOW);  
  
  pinMode(motor, OUTPUT);
  digitalWrite(motor, HIGH);
    
  pinMode(Photo_Switch, OUTPUT);
  digitalWrite(Photo_Switch,HIGH);
  
  setupWiFi();                                                    // WiFi Setup Function Call
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  bot.sendMessage(CHAT_ID, "Setup completed", "");  
  Serial.println("Setup completed");
}

void loop() {
  Check_Wifi();
  Telegram_bot();
  
}
