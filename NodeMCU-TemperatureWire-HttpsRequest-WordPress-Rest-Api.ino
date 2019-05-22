#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Base64.h>
#include <time.h>
#include <TimeLib.h>


//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

// https://github.com/igrr/axtls-8266/blob/master/crypto/hmac.c

#define SHA1_SIZE 20

extern "C" {
  typedef struct
  {
    uint32_t Intermediate_Hash[SHA1_SIZE / 4]; /* Message Digest */
    uint32_t Length_Low;            /* Message length in bits */
    uint32_t Length_High;           /* Message length in bits */
    uint16_t Message_Block_Index;   /* Index into message block array   */
    uint8_t Message_Block[64];      /* 512-bit message blocks */
  } SHA1_CTX;

  void SHA1_Init(SHA1_CTX *);
  void SHA1_Update(SHA1_CTX *, const uint8_t * msg, int len);
  void SHA1_Final(uint8_t *digest, SHA1_CTX *);
}


void ssl_hmac_sha1(const uint8_t *msg, int length, const uint8_t *key, int key_len, uint8_t *digest) {
  SHA1_CTX context;
  uint8_t k_ipad[64];
  uint8_t k_opad[64];
  int i;

  memset(k_ipad, 0, sizeof k_ipad);
  memset(k_opad, 0, sizeof k_opad);
  memcpy(k_ipad, key, key_len);
  memcpy(k_opad, key, key_len);

  for (i = 0; i < 64; i++)
  {
    k_ipad[i] ^= 0x36;
    k_opad[i] ^= 0x5c;
  }

  SHA1_Init(&context);
  SHA1_Update(&context, k_ipad, 64);
  SHA1_Update(&context, msg, length);
  SHA1_Final(digest, &context);
  SHA1_Init(&context);
  SHA1_Update(&context, k_opad, 64);
  SHA1_Update(&context, digest, SHA1_SIZE);
  SHA1_Final(digest, &context);
}

String make_signature(const char* secret_one, const char* secret_two, String base_string) {

  String signing_key = urlencode(secret_one);
  signing_key += "&";
  signing_key += urlencode(secret_two);

  //Serial.println(signing_key);

  uint8_t digestkey[32];
  SHA1_CTX context;
  SHA1_Init(&context);
  SHA1_Update(&context, (uint8_t*) signing_key.c_str(), (int)signing_key.length());
  SHA1_Final(digestkey, &context);

  uint8_t digest[32];
  ssl_hmac_sha1((uint8_t*) base_string.c_str(), (int)base_string.length(), digestkey, SHA1_SIZE, digest);

  String oauth_signature = (base64::encode(digest, SHA1_SIZE).c_str());
  //Serial.println(oauth_signature);

  return oauth_signature;
}

#define ONE_WIRE_BUS D1
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
float temperatur;
const char* ssid     = "your wifi's id";
const char* password = "your wifi's password";
const char* fingerprint = "website's certificate fingerprint";
String oauthConsumerKey = "application's consumer key";
const char* oauthConsumerSecret = "application's consumer secret";
String oauthToken = "webserver's token";
const char* oauthTokenSecret = "webserver's token secret";
String oauthSignatureMethod = "HMAC-SHA1";
String baseString;
String oauthSignature;


int timeZone = 0;
int getMinutes;

void connectWifi() {
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.println("Waiting for connection...");
  }
}

void receiveTime() {
  
  configTime(timeZone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) <= 100000) {

    delay(500);
    Serial.println("Waiting for time...");
  }
}

void setup() {

  Serial.begin(9600);
 
  sensor.begin();

  connectWifi();
  receiveTime();
}

void loop() {

  if (WiFi.status() == WL_CONNECTED) {
    
    time_t timeNow = time(nullptr);    
    getMinutes = minute(timeNow);


    if ( getMinutes == 0 ) {    

      sensor.requestTemperatures(); 
      temperatur = sensor.getTempCByIndex(0); 

      Serial.print("Temperature: ");
      Serial.println(temperatur);
    
      int oauthNonce = random(10000000, 9999999999);  
      
      baseString = "POST&";
      baseString += urlencode("https://") + "www.example.com" + urlencode("/") + "wp-json" + urlencode("/wp/v2/posts/400") + "&";
      baseString += "oauth_consumer_key" + urlencode("=" + oauthConsumerKey);
      baseString += urlencode("&") + "oauth_nonce" + urlencode("=" + String(oauthNonce));
      baseString += urlencode("&") + "oauth_signature_method" + urlencode("=") + oauthSignatureMethod;
      baseString += urlencode("&") + "oauth_timestamp" + urlencode("=" + String(timeNow));
      baseString += urlencode("&") + "oauth_token" + urlencode("=" + oauthToken);
      baseString += urlencode("&") + "oauth_version" + urlencode("=") + "1.0";
      Serial.println(baseString);
      oauthSignature = make_signature(oauthConsumerSecret, oauthTokenSecret, baseString);
      
      
      HTTPClient https; 
      https.setTimeout(100000);
      https.begin("https://www.example.com/wp-json/wp/v2/posts/400", fingerprint ); 
      
      String authorization = "OAuth ";
      authorization += "oauth_consumer_key=";
      authorization += "\"" + urlencode(oauthConsumerKey) + "\", ";
      authorization += "oauth_nonce=";
      authorization += "\"" + urlencode((String)oauthNonce) + "\", ";
      authorization += "oauth_signature_method=";
      authorization += "\"" + oauthSignatureMethod + "\", ";
      authorization += "oauth_timestamp=";
      authorization += "\"" + (String)timeNow + "\", ";
      authorization += "oauth_token=";
      authorization += "\"" + urlencode(oauthToken) + "\", "; 
      authorization += "oauth_version=";
      authorization += "\"1.0\", ";    
      authorization += "oauth_signature=";
      authorization += "\"" + urlencode(oauthSignature) + "\"";
      Serial.println(authorization);      
      https.addHeader("Authorization", authorization); 
      https.addHeader("Content-Type", "application/json");  
      
      String message = "{\"meta\":{\"wt_soil_temperature\":\"" + String(temperatur) + "\",\"wt_timestamp\":\"" + String(timeNow) + "\"}}";
      Serial.println(message);
      
      int httpCode = https.POST(message);
      Serial.println(httpCode); 
      
      // If the request is successful, payload (= WordPress answer) is too big for NodeMCU. It crashes!
      // That's why you should use payload only under debugging.
      // String payload = https.getString();       
      // Serial.println(payload);   
      
      https.end(); 

      ESP.deepSleep(3420000000); 
    }   
  } 
  else {

    connectWifi();
    receiveTime();
  }
}
