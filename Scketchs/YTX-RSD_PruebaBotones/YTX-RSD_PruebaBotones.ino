/* Autor: derfaq (Facundo Daguerre) - Rolling Shutter Displays 
 * Date: 11/6/19
 */

/*
 * Inclusión de librerías. 
 */
 
#include <Kilomux.h>              // Import class declaration
#include <KilomuxDefs.h>          // Import Kilomux defines

#include <RSD.h>
#include <Channel.h>
#include <Screen.h>

// DEFINES PARA EL RSD

//6 = White = ActivateSensorButtonPin
//7 = Green = ActivateSensorLedPin
//8 = Blue  = SensorEchoPin
//9 = Red   = SensorTriggerPin

#define BWIDTH 32 
#define WIDTH ((BWIDTH*8)-1)
#define PIN_R   SensorTriggerPin        // Pin de arduino conectado al LED Rojo
#define PIN_G   ActivateSensorLedPin    // Pin de arduino conectado al LED Verde
#define PIN_B   SensorEchoPin           // Pin de arduino conectado al LED Azul
#define PIN_W   ActivateSensorButtonPin // Pin de arduino conectado al LED Blanco

#define SERIAL_COMMS

// Instancias de objetos //////////////////////////////////////////////////////////////////

Kilomux KmShield;                                       // Objeto de la clase Kilomux

RSD rsd;

Channel red( PIN_R , COMMON_CATHODE , BWIDTH );
Channel green( PIN_G , COMMON_CATHODE , BWIDTH );
Channel blue( PIN_B , COMMON_CATHODE , BWIDTH );
Channel white( PIN_W , COMMON_CATHODE , BWIDTH );

Screen display( &red , &green , &blue );

// Global variables  //////////////////////////////////////////////////////////////////////
unsigned int pot[8];
unsigned int prevPot[8];

bool buttonState[8];
bool buttonLastState[8];
char buttonPushCounter[8];   // counter for the number of button presses

bool led[8];

void setup() { 
  KmShield.init();                                    // Initialize Kilomux shield hardware
  updateKm();

  { // Presentation
  KmShield.digitalWritePortKm( 0x18 , 2 );
  delay(200);
  KmShield.digitalWritePortKm( 0x24 , 2 );
  delay(50);
  KmShield.digitalWritePortKm( 0x42 , 2 );
  delay(50);
  KmShield.digitalWritePortKm( 0x81 , 2 );
  delay(50);
  KmShield.digitalWritePortKm( 0x00 , 2 );
  delay(100);
  KmShield.digitalWritePortKm( 0x81 , 2 );
  delay(100);
  KmShield.digitalWritePortKm( 0x00 , 2 );
  }

  //Setup of the RSD 
  rsd.begin( 30 , BWIDTH );
  
  rsd.attachChannel( red );
  rsd.attachChannel( green );
  rsd.attachChannel( blue );
  rsd.attachChannel( white );

  rsd.attachDraw( draw );

  
#if defined( SERIAL_COMMS )
  Serial.begin( 115200 );
#endif

}

void loop() {
  //Run the RSD engine
  //rsd.update();
  updateKm();
  
  // Tuning: KiloMux way
  int tick = map( KmShield.analogReadKm( MUX_A, 0 ) , 0 , 1023 , rsd.getLowerTick() , rsd.getHigherTick() );
  int fine = map( KmShield.analogReadKm( MUX_A, 1 ) , 0 , 1023 , rsd.getLowerFine() , rsd.getHigherFine() );
  rsd.setTick( tick );
  rsd.setFine( fine );
                                                             
}

//Let's draw!
void draw() {
  display.clear();
  
  //Standarized order of the SMPTE/EBU color bar image : https://en.wikipedia.org/wiki/SMPTE_color_bars
  //from left to right, the colors are white, yellow, cyan, green, magenta, red,  blue and black
  for( int i = 0 ; i <= WIDTH ; i++ ) {
    colour c = ( i * 8 ) / WIDTH;
    display.line( i , c );
  }

  //Grid resolution
  for( int i = 0 ; i < BWIDTH/2 ; i++ ) {
    if( 1 - i%2 ) display.line( i , WHITE );
  }

#if defined(SERIAL_COMMS)
  //Serial diagnosis
  Serial.print("@frsd: ");
  Serial.print( rsd.getFrequency() , 10 );
  Serial.print(" , BWIDHT: ");
  Serial.print(BWIDTH);
  Serial.print(" , tick: ");
  Serial.print( rsd.getTick() );
  Serial.print(" , fine: ");
  Serial.println( rsd.getFine() );
#endif  // endif COMUNICACION_SERIAL
  
  updateKm();
}

void updateKm() {
  // Update pot values
  for( int i = 0 ; i < 8 ; i++ ) {
    prevPot[i] = pot[i];
    pot[i] = KmShield.analogReadKm( MUX_A, i );
  }
  
  // Update button states
  for( int i = 0 ; i < 8 ; i++ ) {
    buttonState[i] = KmShield.digitalReadKm( MUX_B , i , PULLUP );
    
    if ( buttonState[i] != buttonLastState[i] ) {
      if ( buttonState[i] == LOW ) buttonPushCounter[i]++;
    }

    buttonLastState[i]= buttonState[i];
  }

  // Update leds
  for( int i = 0 ; i < 4 ; i++ ) {
    if ( buttonPushCounter[i]&1 ) {
      led[i] = HIGH;
    } else {
      led[i] = LOW;
    }
    KmShield.digitalWriteKm( i + 8 , led[i] );
  }
  
}