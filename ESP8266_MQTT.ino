//ultrason affiche sans fenetre html

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>




int ledPin = D7;
int port = 23;


const byte TRIGGER_PIN = D3; // Broche TRIGGER
const byte ECHO_PIN = D2;    // Broche ECHO
/* Constantes pour le timeout */
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s

/* Vitesse du son dans l'air en mm/us */
const float SOUND_SPEED = 340.0 / 1000;


long measure = 0 ;
float distance_mm = 0;


// Paramètres d'accès au réseau WIFI
const char* APssid = "SFR-3548";
const char* APpassword = "D3DERSIG9XFE";

String myIP = "" ;

// Paramètres de l'Access Point WIFI si pas de connexion
const char* mySSID = "ESPap";
const char* myPassword = "MACONNECT"; // Attention : 8 caractères au minimum


// Création d'une instance de serveur WEB sur le port choisi
const int serverPort = 80 ;
ESP8266WebServer server(serverPort);

// ----------------------------------------------------------------------------
// declaration telnet
// ----------------------------------------------------------------------------
// declare telnet server (do NOT put in setup())
WiFiServer TelnetServer(port);
WiFiClient Telnet;

// ----------------------------------------------------------------------------
// declaration Mosquitto
// https://projetsdiy.fr/esp8266-dht22-mqtt-projet-objet-connecte/
// ----------------------------------------------------------------------------
// declaration mosquitto
#define mqtt_server "192.168.0.17"
#define mqtt_user ""  //s'il a été configuré sur Mosquitto
#define mqtt_password "" //idem


#define data "data"  //Topic data

//Buffer qui permet de décoder les messages MQTT reçus
char message_buff[100];


long lastMsg = 0;   //Horodatage du dernier message publié sur MQTT
long lastRecu = 0;
bool debug = false;  //Affiche sur la console si True


WiFiClient espClient;  // a verifier si on utilise cet objet espClient
PubSubClient client(espClient);



void handleTelnet()
{
      if (TelnetServer.hasClient())
      {
        // client is connected
        if (!Telnet || !Telnet.connected())
        {
          if(Telnet) Telnet.stop();          // client disconnected
          Telnet = TelnetServer.available(); // ready for new client
        } 
        else 
        {
          TelnetServer.available().stop();  // have client, block new conections
        }
      }
    
      if (Telnet && Telnet.connected() && Telnet.available())
      {
        // client input processing
        while(Telnet.available())
          Serial.write(Telnet.read()); // pass through
          // do other stuff with client input here
      } 
}



// ----------------------------------------------------------------------------
// Callback qui traitera les requètes reçues par le serveur WEB
// ----------------------------------------------------------------------------
void handleRoot() 
    {
      server.send(200, "text/html", getPage());
    }

// ----------------------------------------------------------------------------
// Création de la page HTML unique 
// ----------------------------------------------------------------------------
String getPage() 
{

      String res = "<html>" ;
      res += "</h2><h2>measure = " ;
      res += (String)measure ;
      res += "</h2><h2>distance_mm = " ;
      res += (String)distance_mm  ;
      res += " ms</h2>";
      res += "<br><h2><p><center><a href=\"http://";
      res += myIP ;
      res += "\">Refresh</a></center></p></h2>" ;
      res += "</html>" ;
      return res ;

}


// ----------------------------------------------------------------------------
// SETUP : Méthode appelée une seule fois, lors du démarrage
// ----------------------------------------------------------------------------
void setup() 
{
      	// Ouverture du port série, pour l'affichage des messages de mise au point
         Serial.begin(115200) ;

        /* Initialise les broches */
        pinMode(ledPin, OUTPUT);
        pinMode(TRIGGER_PIN, OUTPUT);
        digitalWrite(TRIGGER_PIN, LOW); // La broche TRIGGER doit être à LOW au repos
        pinMode(ECHO_PIN, INPUT);
        delay(500);
      setup_wifi();
      setup_telnet(); // on rends possible la connection a telnet
     setup_mosquitto();
         
      
}
void setup_wifi()
{
         Serial.println("\n\nStarting...") ;
      
         Serial.print("Connexion au réseau WIFI... ");
         int attempts = 1 ;
      
      	// Tentative de connexion sur le SSID de la box
         WiFi.begin(APssid, APpassword);
      
         while ((WiFi.status() != WL_CONNECTED) && (attempts <= 20)) 
                 {
                  delay(500);
              
                  if ((attempts % 10) == 0) Serial.println(".");
                  else Serial.print(".");
                  
                  attempts++ ;
                 }
      
         if (WiFi.status() == WL_CONNECTED) 
                 {
                  	// Si connexion OK
                      Serial.println("");
                      Serial.println("WiFi connected");
                      
                      // Afficher l'IP locale
                      myIP = WiFi.localIP().toString();
                      Serial.println(myIP);
                  
                  	// Optionnel : Déclarer un service sur MDNS
                       if (MDNS.begin("Mon_Serveur")) 
                       {
                        Serial.println("MDNS responder started");
                        MDNS.addService("Mon_Serveur", "tcp", serverPort) ;
                      }   
                } 
        else 
                  {
                    	// Si connexion NOK
                        Serial.print("AP Mode : SSID = ");
                        Serial.println(mySSID);
                    	
                    	// Passer en mode Access Point WIFI sur le SSID "mySSID". IP 192.168.4.1
                        WiFi.softAP(mySSID, myPassword);
                      
                        IPAddress myIP = WiFi.softAPIP();
                        Serial.print("=> AP IP address: ");
                        Serial.println(myIP);
                    }
        // Définir le callback du serveur WEB
        server.on("/", handleRoot);
        server.begin();
        Serial.println("Server started");  
}
 ///////////////////////////////////////////////////
        //// call back //////////////////////////////////
     ////////////////////////////////////////////////
    // Déclenche les actions à la réception d'un message
    // D'après http://m2mio.tumblr.com/post/30048662088/a-simple-example-arduino-mqtt-m2mio
    void callback(char* topic, byte* payload, unsigned int length) 
    {
    
            int i = 0;
            if ( debug ) {
              Serial.println("Message recu =>  topic: " + String(topic));
              Serial.print(" | longueur: " + String(length,DEC));
            }
            
            // create character buffer with ending null terminator (string)
            for(i=0; i<length; i++) 
            {
              message_buff[i] = payload[i];
            }
            message_buff[i] = '\0';
            
            String msgString = String(message_buff);
            if ( debug )
            {
              Serial.println("Payload: " + msgString);
            }
          
            if ( msgString == "ON" )
            {
              digitalWrite(ledPin,HIGH);  
            } else 
            {
              digitalWrite(ledPin,LOW);  
            }
    }
 void setup_mosquitto()
 {      
      //partie mosquitto
        client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
        client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message   
}



// ----------------------------------------------------------------------------
// telnet 
// ----------------------------------------------------------------------------
void setup_telnet ()
{
        // partie telnet
         TelnetServer.begin();
        Serial.print("Starting telnet server on port " + (String)port);
        // TelnetServer.setNoDelay(true); // ESP BUG ?
        Serial.println();
        delay(100);
      
         
         Serial.println("Telnet started");
        // Faire clignoter la LED
        for(int i = 0 ; i < 6 ; i++) 
                          {
                            digitalWrite(ledPin, LOW);
                            delay(500);
                            digitalWrite(ledPin, HIGH);
                            delay(300);     
                          }
}
  
     // ----------------------------------------------------------------------------
// Reconnection
// ----------------------------------------------------------------------------
void reconnect() 
      {
            //Boucle jusqu'à obtenir une reconnexion
            while (!client.connected()) 
            {
                Serial.print("Connexion au serveur MQTT...");
                if (client.connect("ESP8266Client", mqtt_user, mqtt_password))
                  {
                    Serial.println("OK");
                  }
                else 
                  {
                    Serial.print("KO, erreur : ");
                    Serial.print(client.state());
                    Serial.println(" On attend 5 secondes avant de recommencer");
                    delay(5000);
                  }
            }
      }
// ----------------------------------------------------------------------------
// Methode LOOP : Appelée en boucle, permanence...
// ----------------------------------------------------------------------------
void loop() 
{
    
    
            digitalWrite(TRIGGER_PIN, HIGH);
            delayMicroseconds(10);
            digitalWrite(TRIGGER_PIN, LOW);
            
            /* 2. Mesure le temps entre l'envoi de l'impulsion ultrasonique et son écho (si il existe) */
            measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
             
            /* 3. Calcul la distance à partir du temps mesuré */
            distance_mm = measure / 2.0 * SOUND_SPEED;
           
            Serial.println("la distance :");
            Serial.print(distance_mm);
          
            Serial.println("la mesure");
            Serial.println(measure);
            Serial.println (" y a plus qu a les retrouve sur ta page");


              ///////////////////////////////////////////////////
              //// partie Telnet loop //////////////////////////////////
              ////////////////////////////////////////////////
            // Traiter les requètes arrivant sur le serveur WEB [le callback sera appelé pour chaque requète reçue]
            server.handleClient();
             handleTelnet();
             //  Telnet.print(ansiHOME+ansiCLC); // clear screen
             Telnet.println("la distance : "+ (String)distance_mm + "la mesure" +(String) measure );
             delay(2000); // to fast might crash terminals
    

      
              ///////////////////////////////////////////////////
              //// partie Mosquito loop //////////////////////////////////
              ////////////////////////////////////////////////
    
     if (!client.connected()) 
     {
        reconnect();
      }
      client.loop();
    
    long now = millis();
      //Envoi d'un message par minute
      if (now - lastMsg > 1000 * 60) 
      {
        lastMsg = now;
      }
      
      //Inutile d'aller plus loin si le capteur ne renvoi rien
     if ( isnan(distance_mm) || isnan(measure)) 
     {
      Serial.println("Echec de lecture ! Verifiez votre capteur DHT");
      return;
     }
       client.publish(data, String(distance_mm).c_str(), true);   //Publie la distance sur le topic data
        client.publish(data, String(measure).c_str(), true);      //Et la mesure sur le topic data
        delay(3000);


}
