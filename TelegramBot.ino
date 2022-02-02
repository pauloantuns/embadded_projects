#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <math.h>

const char* ssid = "Fenix";
const char* pass = "x1e3pjitbav";

const int randomico = random(1000);

const int sensor = 4;
#define DHTTYPE DHT11
DHT dht(sensor, DHTTYPE);

#define BMP_SDA 21
#define BMP_SCL 22

Adafruit_BMP280 bmp280;

//#define CHAT_ID "1677067715"
//#define CHAT_ID "-1001659044240" //canal
#define BOTtoken "5092819419:AAFqrgzfx68Po1VfTHTivdW3_9ByEQaH-YU"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int botRequestDelay = 750;
unsigned long lastTimeBotRan;

const float b = 243.12;
const float a = 17.62;

const int maxloop = 72;
int passo = 0;
int n = 0;
const int intervalo = 600000; //10minutos
int maxinterval = intervalo/2500;

float *leituras(){
  static float dados[5];
  
  float temp = bmp280.readTemperature();
  float humi = dht.readHumidity();
  float pres = (bmp280.readPressure()/100);
  float hic  = dht.computeHeatIndex(temp, humi, false);
  dados[0] = temp;
  dados[1] = humi;
  dados[2] = pres;
  dados[3] = hic;

  float alpha = log(humi/100) + a*temp/(b+temp);
  float Td = (b*alpha)/(a - alpha);
  dados[4]  = Td;
 
  return dados;
}

void handleNewMessages(int numNewMessages){
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    Serial.println(chat_id);
   // if (chat_id != CHAT_ID){
  //    bot.sendMessage(chat_id, "Usuário não autorizado a utilizar este bot.", "");
  //    continue;
   // }

  String text = bot.messages[i].text;
  Serial.println(text);
  String from_name = bot.messages[i].from_name;

  if(text == "/ola" or text == "/start"){
    String bemvindo = "Bem vindo, " + from_name + "!\n";
    bemvindo += "Eu envio dados metereológicos.\n";
    bot.sendMessage(chat_id, bemvindo, "");
  }

  if(text == "/ajuda" or text == "/comandos"){
    String ajuda = "Olá " + from_name + " aqui estão os comandos disponíveis: \n\n";
      ajuda += "/ola ... Mensagem de boas vindas.\n";
      ajuda += "/ajuda ... Exibe os comandos disponíveis.\n";
      ajuda += "/agora ... Envia uma única leitura dos dados metereológicos, em tempo real. \n";
      
  if(chat_id == "1677067715"){
      ajuda += "/loop ... Bot entra em loop, enviando dados a cada 10 min pelas próximas 12 horas. \n";
      ajuda += "/reboot ... Reinicia o bot. \n";
      ajuda += "/parar ... Interrompe o loop de envio de dados. \n";
      bot.sendMessage(chat_id, ajuda, "");
    }
    else{
      bot.sendMessage(chat_id, ajuda, "");
    }
  }

  if(text == ("/reboot" + String(randomico)) and chat_id == "1677067715"){
      ESP.restart();
  }

  if(text == "/agora"){
    float* dados = leituras();
      String message = "Temperatura: " + String(dados[0]) + "°C \n";
      message += "Umidade Relativa do Ar: " + String(dados[1]) + "% \n";
      message += "Indice de Calor: " + String(dados[3]) + "°C \n"; 
      message += "Pressão Atmosférica Local: " + String(dados[2]) + "hPa \n";
      message += "Temperatura do Ponto de Orvalho: " + String(dados[4]) + "°C \n";
      
     bot.sendMessage(chat_id, message, "");
  }
  
  if(text == "/loop" and chat_id == "1677067715"){
    passo = 0;
    while(passo < maxloop){
      
      float* dados = leituras();
      String message = "Temperatura: " + String(dados[0]) + "°C \n";
      message += "Umidade Relativa do Ar: " + String(dados[1]) + "% \n";
      message += "Indice de Calor: " + String(dados[3]) + "°C \n"; 
      message += "Pressão Atmosférica Local: " + String(dados[2]) + "hPa \n";
      message += "Temperatura do Ponto de Orvalho: " + String(dados[4]) + "°C \n";
      
      bot.sendMessage("-1001659044240", message, "");
      passo++;
      //bot.sendMessage("1677067715", message, "");
      while(n < (maxinterval/2)){
        delay(2500);
        n++;
        loop(); 
      }
      if(n == 0.5*maxinterval){
        n = 0;
      }
    }
  }

   if((text == "/parar" or text == "/stop") and chat_id == "1677067715"){
    String parar = "Loop interrompido. Aguardando novas instruções. \n";
    bot.sendMessage("1677067715", parar, "");
    passo = maxloop;
    continue;
   }
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint32_t naoainda = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi..");
    naoainda++;
    if(naoainda > 127){
      Serial.println("Resetting due to Wifi not connecting...");
      ESP.restart();
    }
  }
  
  dht.begin();
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  boolean status = bmp280.begin(0x76);
  if (!status) {
    Serial.println("Não conectado");
  }
  String online = "Olá, estou online!";
  online += "\n " + String(randomico);
  bot.sendMessage("1677067715", online, "");

}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
