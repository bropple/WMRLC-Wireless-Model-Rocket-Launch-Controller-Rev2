/*  BEN-CO Wireless Model Rocket Launch Controller (MRLC)
 *  Ben Ropple
 *   
 *   This code and the accompanied schematic is fully open source. Modify the code
 *   and the circuitry to your liking, but also at your own risk!
 *   
 *   External Libraries used:
 *   
 *   MCP342x by Steve Marple - for communicating with MCP3426 I2C ADC
 *   https://github.com/stevemarple/MCP342x
 *   
 *   ESPAsyncWebServer by me-no-dev - Asynchronus HTTP and Websocket server
 *   https://github.com/me-no-dev/ESPAsyncWebServer
 *   
 */

#include <Wire.h>
#include <MCP342x.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#define RELAY D8 //controls a NPN BJT to open relay when HIGH
#define ADC_INT A0 //internal ADC used only for internal 5V battery level checking   

Ticker relayCtrl; //used as a timer for the igniter burn

uint8_t ADC_addr = 0x68; //I2C address of the ADC module, may be different
MCP342x ADC = MCP342x(ADC_addr);

MCP342x::Config status;

bool buttonPushed = false;
bool isIgniter = false;
bool firing = false;
//bool isTickerOn = false;

int RELAY_ONTIME = 5; //the number of seconds the relay is toggled on.

int contState = 0; //keeps track of continuity state

bool contChecked = false; //has the continutity been checked once?

bool firstRun = true; //loop running for first time

long ADCvalue; //the raw ADC voltage value
long ADCtimeout = 2000000; //Maximum ADC conversion time


//note: the formula for ADC conversion is (ADC resolution) / (reference voltage) = (ADC reading)/(Analog Voltage)
//the resolution of the internal ADC is 1023, with a reference of ~1.023V
//the resolutuin of the I2C ADC is 32767, with a reference of 2.048V
float calibration = 15999.51; //From ADC conversion formula: 32767/2.048 = (adc reading)/(analog voltage)
float int_calibration = 1000; //10 bit internal ADC calibration 1023/1.024 = (adc reading)/(analog voltage)

//These calibration values were obtained via simulation, you may need to alter them if your results are wrong
float FiveV_calibration = 5.16; //Multiplier to obtain the 5V battery voltage (at the voltage divider)
float Ignition_calibration = 7.66479; //Multiplier to obtain the original ignition battery voltage

double Get_Diff_Voltage(void){
  /* This function retrieves the ADC value of the voltage present across the igniter and converts
   * it to an actual voltage value.
   * 
   * Arguments: none
   * Returns: Voltage across igniter (double)
   * 
   */

  ADCvalue = 0;

  uint8_t err = ADC.convertAndRead(MCP342x::channel2, MCP342x::oneShot,
          MCP342x::resolution16, MCP342x::gain1,
          ADCtimeout, ADCvalue, status);
          
  if(err){
    Serial.print(F("ADC Conversion Error!"));
    Serial.println(err);
    }
  else{
    double DiffVoltage = ((double)ADCvalue / calibration);
    Serial.print(F("ADC Differential Value: "));
    Serial.println(ADCvalue);
    Serial.print(F("Actual Differential Voltage: "));
    Serial.print(DiffVoltage, 5);
    Serial.println(F("V"));

    MCP342x::generalCallReset();
    delay(5);
    
    return DiffVoltage;
  }
  return 999999.0; //an error has occured!
}

double Get_5V_Voltage(void){
  /*  This function retrieves the ADC value of the voltage at the 5V battery voltage divider
   *  and convert it into an actual voltage value
   *  
   *  Arguments: none
   *  Returns: Voltage value (double)
   * 
   */

  ADCvalue = analogRead(A0);

  double FiveV_Vdiv_Voltage = ((double)ADCvalue / int_calibration);
  double FiveV_Voltage = FiveV_Vdiv_Voltage * FiveV_calibration;
  Serial.print(F("ADC 5V Battery Value: "));
  Serial.println(ADCvalue);
  Serial.print(F("Voltage at the Voltage Divider: "));
  Serial.print(FiveV_Vdiv_Voltage, 5);
  Serial.println(F("V"));
  Serial.print(F("Actual 5V Battery Voltage: "));
  Serial.print(FiveV_Voltage, 5);
  Serial.println(F("V"));

  delay(5);
  
  return FiveV_Voltage;

  
  //return 999999.0; //an error has occured!
}

double Get_Ignition_Voltage(void){
  /* This function retrives the ADC value of the ignition battery voltage
   * and returns it as an actual voltage.
   * 
   * Arguments: none
   * Returns: Ignition battery voltage (double)
   * 
   */

  ADCvalue = 0;

  uint8_t err = ADC.convertAndRead(MCP342x::channel1, MCP342x::oneShot,
          MCP342x::resolution16, MCP342x::gain1,
          ADCtimeout, ADCvalue, status);

  if(err){
    Serial.print(F("ADC Conversion Error!"));
    Serial.println(err);
   }
  else{
    double Ignition_Vdiv_Voltage = ((double)ADCvalue / calibration);
    double Ignition_Voltage = Ignition_Vdiv_Voltage * Ignition_calibration;
    Serial.print(F("ADC Ignition Battery Value: "));
    Serial.println(ADCvalue);
    Serial.print(F("Voltage at the Voltage Divider: "));
    Serial.print(Ignition_Vdiv_Voltage, 5);
    Serial.println(F("V"));
    Serial.print(F("Actual Ignition Battery Voltage: "));
    Serial.print(Ignition_Voltage, 5);
    Serial.println(F("V"));
    MCP342x::generalCallReset();
    delay(5);

    return Ignition_Voltage;
  }
  MCP342x::generalCallReset();
  delay(5);
  
  return 999999.0; //an error has occured!
}

uint8_t Continuity_Check(void){
  /* This function calls all the ADC functions above and determines the continuity state.
   *  
   *  Arguments: none
   *  Returns: Status code (uint8_t)
   * 
   */
  
  Serial.println();
  Serial.println(F("Continuity Check"));
  Serial.println(F("----------------"));

  //Get all the measurements we need to determine if a igniter is inserted
  double DiffVoltage = Get_Diff_Voltage();
  double FiveV_Voltage = Get_5V_Voltage();
  double Ignition_Voltage = Get_Ignition_Voltage();
  
  if((DiffVoltage == 999999.0) || (FiveV_Voltage == 999999.0) || (Ignition_Voltage == 999999.0)){
    Serial.println(F("ADC Measurement Error!"));
    return -1; //hardware failure
  }
  if(Ignition_Voltage <= 1.0){ //this low of a voltage 
    Serial.println(F("Launch battery not present. Aborting!"));
    return 0; //no launch battery connected at all
  }
  if(Ignition_Voltage <= 5.8){ //Igniter needs at least 6V 2A to burn
    Serial.println(F("Insufficient voltage to ignite!"));
    return 1; //launch battery voltage is insufficient
  }
  if(FiveV_Voltage <= 4.0){ //If 5V supply is too low
    Serial.println(F("Internal 5V battery level is too low!"));
    return 2; //can only happen when serial USB is connected, because 5V measurement comes from VIN pin, not VU pin
  }

  else if(DiffVoltage <= 0.0015){ //If the voltage difference is incredibly small, or the same
    Serial.println(F("Short Circuit!"));
    return 3; //short circuit condition
  }
  else if(DiffVoltage >= 0.5){ //If the voltage difference is large -> high resistance or open
    Serial.println(F("Open Circuit!"));
    return 4;  //open circuit condition
  }

  Serial.println(F("Continuity PASS")); //if everything checks out, allow for launch
  Serial.println();
  return 5; //continuity PASS condition
}

//website stuff
const char* ssid = "BEN-CO_Rocket_Launch_Ctrlr"; //Network SSID, the name of the WiFi network as seen on client device
const char* password = "ROCKETMAN";

IPAddress local_ip(8,8,8,8); //DNS based auto-login
IPAddress gateway(8,8,8,8);
IPAddress subnet(255,255,255,0);
const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer server(80);

bool Allow_ContCheck = false; //check the continuity in loop();
bool ContCheck_PASS = false; //keep track of if continuity check passed or not
bool Allow_Launch = false; //Launch the rocket by toggling relay in loop();
                           //relay will shut itself off after 5 seconds due to Ticker function.

/*
 * The below const char is the HTML, CSS, and JavaScript content of the site.
 */
const char html[] PROGMEM = R"rawliteral(
                    <!DOCTYPE html> <html>
                      <head>
                        <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
                        <title>Launch Controller</title>
                         <style> html { background-color: #282828; font-family: monaco, helvetica; display: inline-block; margin: 0px auto; text-align: center;}
                                  body {margin-top: 25px; user-select: none;} h1 {color: #FFFFFF;margin-left: 5px auto 30px; margin-right: 5px auto 30px; margin-bottom: 5px;} h2 {color: #FFFFFF;margin-bottom: 10px; margin-top: 15px;} h3 {color: #FFFFFF;margin-bottom: 25px; margin-top: 1px;} h4 {font-size: 1.5em; color: #FFFFFF;margin-bottom: 2px; margin-top: 65px;}
                                  .cont-button {display: block; width: 150px; background-color: #1abc9c;border: 1px solid white;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 10px auto 15px; text-align: center; display: inline; cursor: pointer; user-select: none; border-radius: 12px;}
                                  .cont-button:hover{box-shadow: 0 12px 16px 0 rgba(0,0,0,0.24), 0 17px 50px 0 rgba(0,0,0,0.19);}
                                  .launch-button {display: block;width: 180px; height: 180px; border: 3px solid gold; background-color: #ff0000; color: white; text-decoration: none;font-size: 25px;text-align: center; display: inline-block; user-select: none; border-radius: 50%;}
                                  .button-checkCont {background-color: #0053ff;}
                                  .button-noCont {background-color:  #ff0000;}
                                  .button-Cont{background-color: #0aff00;}
                                  .button-noLaunch {opacity: 0.6; cursor: not-allowed;}
                                  .button-Launch{cursor: pointer;}
                                  .launch-bg{width: 180px; height: 180px; border-radius: 15px; background-color: #7d7d7d; border: 1px solid white; display: inline-block; text-align: center;}
                                  p {font-size: 1em;color: #FFFFFF; margin-bottom: 2px; margin-top: 1px; text-align: center; }
                         </style>
                      </head>
                      <body>
                        <h1>BEN-CO&#8482; Model Rocket Launch Controller</h1>
                        <h3>Version 1.0</h3>
                        <h2>Check Continuity</h2>
                          <input type="button" id="cont" class="cont-button button-checkCont" onmousedown="buttonCheck(this)" ontouchstart="buttonCheck(this)" onmouseup="setTimeout(doubleCheck.bind(null, this), 500)" ontouchend="setTimeout(doubleCheck.bind(null, this), 500)"  value="CHECK" ></input>
                          <h2>Launch</h2>
                          <div class="launch-bg">                
                            <input type="button" id="launch" class="launch-button button-noLaunch" value="DISABLED" onmousedown="null" ontouchstart="null" onmouseup="null" ontouchend="null" ></input>
                          </div>
                        <h4>System Messages</h4>
                          <p id="ContMsg">
                            Check continuity before launching!
                          </p>
                          <p id="canLaunch">
                            Launch button is disabled!
                          </p>
                          <p id="launchMessage">
                            Error messages will appear here if any occur.
                          </p>
                          <script>
                            var DoubleCheck = false;
                            function buttonCheck(element){
                              var buttonID = element.id;
                              var xhr = new XMLHttpRequest();
                              xhr.responseType="text";                       
                              if(buttonID = "cont") xhr.open("GET", "/ButtonEvent?cupd=1", true);
                              else if(buttonID = "launch" && DoubleCheck == true) xhr.open("GET", "/ButtonEvent?lupd=1", true);
                              xhr.onreadystatechange = checkData;                      
                              xhr.send();
                              var state;
                              var restxt;
                              function checkData(){
                                state = xhr.readyState;
                                console.log(state);
                                if(state == 4){
                                  restxt = xhr.responseText;
                                  console.log(restxt);
                                  //console.log(restxt == "CHECKING");
                                  if(restxt != "NO ACTION"){
                                     DoubleCheck = true;
                                  }
                                }
                              }
                              //setTimeout(refreshPage(), 1000);
                            }
                            function doubleCheck(element){
                              if(DoubleCheck == true){
                                var buttonID = element.id;
                                var xhr2 = new XMLHttpRequest();
                                xhr2.responseType="text";                       
                                if(buttonID == "cont") xhr2.open("GET", "/updateEvent?cupd=1", true);
                                else if(buttonID == "launch") xhr2.open("GET", "/updateEvent?lupd=1", true);
                                xhr2.onreadystatechange = checkAgain;                      
                                xhr2.send();
                                var state2;
                                var restxt3;
                                  function checkAgain(){
                                    state2 = xhr2.readyState;
                                    console.log(state2);
                                    if(state2 == 4){
                                      restxt3 = xhr2.responseText;
                                      console.log("Second Check");
                                      console.log(restxt3);
                                      if(buttonID == "launch"){
                                        if(restxt3 == "PASS") launchButton(element);
                                        else {
                                          updateButtons(restxt3);
                                          return restxt3;
                                        }
                                        DoubleCheck = false;
                                        return restxt3;
                                      }
                                      updateButtons(restxt3);
                                      return restxt3;
                                    }
                                  }
                               }
                             else return "CHK2 FAIL";
                             DoubleCheck = false;
                            }
                            function updateButtons(TextCode){
                              console.log("updateButtons has been called.");
                              var cbutton = document.getElementById("cont");
                              var lbutton = document.getElementById("launch");
  
                              var cmsg = document.getElementById("ContMsg");
                              var CLmsg = document.getElementById("canLaunch");
                              var Lmsg = document.getElementById("launchMessage");
                              
                              if(TextCode == "PASS"){
                                cbutton.className="cont-button button-Cont";
                                cbutton.value = "PASS"; //change the continuity button text to PASS             
                                lbutton.value = "READY"; //change launch button text to READY
                                lbutton.className="launch-button button-Launch";
                                lbutton.setAttribute("onmousedown","buttonCheck(this)");
                                lbutton.setAttribute("ontouchstart","buttonCheck(this)");
                                lbutton.setAttribute("onmouseup","setTimeout(doubleCheck.bind(null, this), 500)");
                                lbutton.setAttribute("ontouchend","setTimeout(doubleCheck.bind(null, this), 500)");
  
                                cmsg.innerHTML="Continuity PASS!";
                                CLmsg.innerHTML="Ready to Launch!";
                                Lmsg.innerHTML="No problems detected.";
                              }
                              else{
                                cbutton.className="cont-button button-noCont";
                                cbutton.value = "FAIL";
                                cmsg.innerHTML= "Continuity FAIL!";
  
                                lbutton.value = "DISABLED";
                                lbutton.className="launch-button button-noLaunch";
                                lbutton.setAttribute("onmousedown","null");
                                lbutton.setAttribute("ontouchstart","null");
                                lbutton.setAttribute("onmouseup","null");
                                lbutton.setAttribute("ontouchend","null");
                                CLmsg.innerHTML="Launch button is disabled!";
  
                                if(TextCode == "ADC_FAIL") Lmsg.innerHTML="ADC Conversion Failure!";
                                else if(TextCode == "NO_IGBAT") Lmsg.innerHTML="Please connect a 6-12V ignition battery!";
                                else if(TextCode == "BAD_IGBAT") Lmsg.innerHTML="Ignition battery voltage is too low!";
                                else if(TextCode == "BAD_5VBAT") Lmsg.innerHTML="Internal battery voltage is too low! Is the switch on?";
                                else if(TextCode == "SHORT") Lmsg.innerHTML="Short circuit! Check igniter connection!";
                                else if(TextCode == "OPEN") Lmsg.innerHTML="Open circuit or High Resistance! Check igniter connection!";
                                else if(TextCode == "UNK_ERR") Lmsg.innerHTML="An unknown error has occured!";
                              }
                            }
                            function launchButton(element){
                              console.log("Launching rocket!");
                              var xhr = new XMLHttpRequest();
                              xhr.open("GET", "/ButtonEvent?launch=1", true);
                              xhr.send();     
                              setTimeout(resetButtons, 6000); //reset buttons back to original states after rocket has fired         
                            }
                            function resetButtons(){
                              console.log("resetButtons has been called.");
                              var cbutton = document.getElementById("cont");
                              var lbutton = document.getElementById("launch");
                              
                              var cmsg = document.getElementById("ContMsg");
                              var CLmsg = document.getElementById("canLaunch");
                              var Lmsg = document.getElementById("launchMessage");
                             
                              cbutton.value = "CHECK";
                              cbutton.className="cont-button button-checkCont";
                              cmsg.innerHTML="Check continuity before launching!";
                           
                              lbutton.value = "DISABLED";
                              lbutton.className="launch-button button-noLaunch";
                              lbutton.setAttribute("onmousedown","null");
                              lbutton.setAttribute("ontouchstart","null");
                              lbutton.setAttribute("onmouseup","null");
                              lbutton.setAttribute("ontouchend","null");
                              CLmsg.innerHTML="Launch button is disabled!";
                              Lmsg.innerHTML="Error messages will appear here if any occur."; //maybe make this say the launch is successful later
                            }
                          </script>
                      </body>
                   </html>
)rawliteral";

uint8_t LEDpin = LED_BUILTIN; //the built-in blue/white LED on the NODEMCU
bool LEDstatus = HIGH;

void setup() {
  //testing
  Serial.begin(115200); //serial monitor available at 115200 baud
  Wire.begin(5, 4); //need to use pin numbers 4 and 5 verbosely or it will not work!

  //testing
  //pinMode(TESTBTN, INPUT_PULLUP);
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, HIGH);

  Wire.requestFrom(ADC_addr, (uint8_t)1);
  if (!Wire.available()) {
    Serial.print("No device found at I2C address ");
    Serial.println(ADC_addr, HEX);
    while (1); //causes board to crash, but still notifies user of problem via serial
  }

  MCP342x::generalCallReset();
  delay(5);
  
  //pin setup
  pinMode(RELAY, OUTPUT); //set relay output pin to an output
  pinMode(ADC_INT, INPUT_PULLUP); //set internal ADC pin A0 to an input_pullup to avoid having a potentially floating node
  
  //initially set everything LOW
  digitalWrite(RELAY, LOW);

  //WiFi Setup

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1, false, 1); //only allow one user to log in at a time.
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", local_ip);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    LEDstatus = LOW; //turn LED off on successful WiFi Connection
    request->send_P(200, "text/html", html);
  });
  
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", html); //Android Captive Portal
  });
  
  server.on("/404", HTTP_GET, [](AsyncWebServerRequest *request){
    //handles 404's
    request->send_P(200, "text/plain", "404 Not Found");
  });

    server.on("/updateEvent", HTTP_GET, [] (AsyncWebServerRequest *request) {
      //this sends codes to the client device based on the continuity return value.
    if(request->hasParam("cupd") || request->hasParam("lupd")){
      switch(contState){
        case -1: request->send(200, "text/plain", "ADC_FAIL");
        case 0: request->send(200, "text/plain", "NO_IGBAT");
        case 1: request->send(200, "text/plain", "BAD_IGBAT");
        case 2: request->send(200, "text/plain", "BAD_5VBAT");
        case 3: request->send(200, "text/plain", "SHORT");
        case 4: request->send(200, "text/plain", "OPEN");
        case 5: request->send(200, "text/plain", "PASS");
        default: request->send(200, "text/plain", "UNK_ERR");
      }
    }
  });

  server.on("/ButtonEvent", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/updateCont?cupd=<inputMessage>
    if (request->hasParam("cupd") || request->hasParam("lupd")) {
      //allow continuity function to run in loop, the only place it runs without failing...
      inputMessage = "Authorizing continuity check.";
      Allow_ContCheck = true; //Authorize a continuity check
      request->send(200, "text/plain", "CHECKING");
    }
    else if (request->hasParam("launch")) {
      inputMessage = "Authorizing launch!";
      Allow_Launch = true; //still checks continuity again before actually launching it
      request->send(200, "text/plain", "LAUNCHING");
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
      request->send(200, "text/plain", "NO ACTION");
    }
    Serial.println(inputMessage);
  });
  
  server.begin();
  Serial.println("HTTP server started!");
  
 }

void tickerRelay(void){ // this turns the ignition relay OFF after it times out
 digitalWrite(RELAY, LOW);
}

void loop() {
  dnsServer.processNextRequest(); //needed for captive portal

  //don't have delays for too long or the captive portal won't work!
  
  if(LEDstatus){ //LED turns on at boot, turns off upon login and first access to page
    digitalWrite(LEDpin, LOW);
  }
  else{
    digitalWrite(LEDpin, HIGH);
  }

  if(firing == false){
    if(Allow_Launch == true){ //If launching is authorized
      
      //Get_Ignition_Voltage();
      contState = Continuity_Check(); //check continuity
      
    if(contState == 5) { //if the continuity passes
       isIgniter = true; //there is an igniter
    }
    else {
      isIgniter = false; // no igniter inserted.
      ContCheck_PASS = false; //Reset passed continuity check flag
      Allow_Launch = false; //Revoke launch authorization
    }
      
      if(isIgniter == true){ //if there is an igniter
        relayCtrl.attach(RELAY_ONTIME, tickerRelay); //relay timer using the hardware timer
        digitalWrite(RELAY, HIGH); //turn relay ON
        firing = true; //system is firing the rocket.
      }
      else{
        firing = false; //system is not firing the rocket.
      }
      ContCheck_PASS = false; //Reset passed continuity check flag
      Allow_Launch = false; //Revoke launch authorization
      contState = 0; //revert state back to its original value
    }
  }

  if(Allow_ContCheck == true){
    //bool cont = false;
    contState = Continuity_Check(); //the first manual continuity check after the continuity button is pressed
    if(contState == 5) {
      ContCheck_PASS = true; //remembers that continuity has been manually checked and passed
    }
    else ContCheck_PASS = false; //continuity check has failed
    Allow_ContCheck = false; //Revoke continuity check authorization
    Serial.print("ContCheck_PASS: ");
    Serial.println(ContCheck_PASS);
    Serial.print("contState: ");
    Serial.println(contState);
  }
  
  if(digitalRead(RELAY) == LOW) {
    relayCtrl.detach(); //turns the relay hardware timer off if the relay is set to LOW
    firing = false; //system isn't firing
  }
}
