/* Autor: derfaq (Facundo Daguerre) - Rolling Shutter Displays 
 * Date: 05/04/20
 */
 
/*  Layout /////////////////////////////////////////////////////////////////////////////
 * 
 * ╔══════════════════════════════════════╗
 * ║                                                                 ║
 * ║   Main     Sintonía                                             ║
 * ║   Param    Gruesa   Fina     Shift                              ║
 * ║     ○        ○        ○        ○                              ║
 * ║    p[0]     p[1]     p[2]     p[3]              RSD             ║
 * ║   Param    Param    Param    Param               ☼              ║
 * ║     x        x        x        x                                ║
 * ║     ○        ○        ○        ○                              ║
 * ║    p[5]     p[6]     p[7]     p[8]                              ║
 * ║                                                                 ║
 * ║     On       Bloq.                                              ║
 * ║     Off      Freq      x        x                               ║
 * ║     .        .         .        .                               ║
 * ║     ■        ■        ■        ■                              ║
 * ║                                                                 ║
 * ║                                                                 ║               
 * ║                                                                 ║
 * ║     x        x        x        x                                ║
 * ║     .        .        .        .                                ║
 * ║     ■        ■        ■        ■                              ║
 * ║                                                                 ║
 * ╚══════════════════════════════════════╝
 * 
 */

// Libraries  ////////////////////////////////////////////////////////////////////////////
 
#include <Kilomux.h>              // Import class declaration
#include <KilomuxDefs.h>          // Import Kilomux defines

#include <RSD.h>
#include <Channel.h>
#include <Screen.h>

#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();


// Definitions ////////////////////////////////////////////////////////////////////////////

// 6 = White = ActivateSensorButtonPin
// 7 = Green = ActivateSensorLedPin
// 8 = Blue  = SensorEchoPin
// 9 = Red   = SensorTriggerPin
// TODO: Pin-out

#define BWIDTH 32
#define WIDTH ((BWIDTH*8)-1)
#define PIN_R   SensorTriggerPin        // Pin de arduino conectado al LED Rojo
#define PIN_G   ActivateSensorLedPin    // Pin de arduino conectado al LED Verde
#define PIN_B   SensorEchoPin           // Pin de arduino conectado al LED Azul
#define PIN_W   ActivateSensorButtonPin // Pin de arduino conectado al LED Blanco

// Objets /////////////////////////////////////////////////////////////////////////////////

Kilomux KmShield;                                       // Objeto de la clase Kilomux

RSD rsd;

Channel red( PIN_R , COMMON_CATHODE , BWIDTH );
Channel green( PIN_G , COMMON_CATHODE , BWIDTH );
Channel blue( PIN_B , COMMON_CATHODE , BWIDTH );
Channel white( PIN_W , COMMON_CATHODE , BWIDTH );

Screen display( &red , &green , &blue );

Channel *ch[4] = { &red , &green , &blue , &white  };

// Global variables  //////////////////////////////////////////////////////////////////////

unsigned int pot[8];
unsigned int prevPot[8];

bool buttonState[8];
bool buttonLastState[8];
char buttonPushCounter[8];   // counter for the number of button presses

bool led[8];

bool bloq = false;

//  Beginnig  /////////////////////////////////////////////////////////////////////////////

void setup() { 
  //Setup of Kilomux 
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


  //Setup of RSD 
  rsd.begin( 30 , BWIDTH );
  
  rsd.attachChannel( &red );
  rsd.attachChannel( &green );
  rsd.attachChannel( &blue );
  rsd.attachChannel( &white );

  rsd.attachDraw( draw );

  //Comunications
  MIDI.setHandleNoteOn( handleNoteOn ); 
  MIDI.setHandleNoteOff( handleNoteOff );
  
  MIDI.begin( MIDI_CHANNEL_OMNI );


}

// For ever  ///////////////////////////////////////////////////////////////////////////

void loop() {
  //Run the RSD engine
  rsd.update();
  
  if ( !bloq ) {
    //Tuning: Kilomux way
    int thick = map( KmShield.analogReadKm( MUX_A, 0 ) , 0 , 1023 , rsd.getLowerThick() , rsd.getHigherThick() );
    int fine = map( KmShield.analogReadKm( MUX_A, 1 ) , 0 , 1023 , rsd.getLowerFine() , rsd.getHigherFine() );
    rsd.setThick( thick );
    rsd.setFine( fine );
  }
  
  //MIDI update
  MIDI.read();                                                    
}

bool kick = false;
unsigned char wide = 0;
colour pallete[2] = { YELLOW , RED };
colour c;
unsigned char counterColour = 0;

// Let's draw! //////////////////////////////////////////////////////////////////////////
void draw() {
  
  display.clear();
  white.clear();

  if( kick ) display.fill( WIDTH/2 - wide , WIDTH/2 + wide , pallete[counterColour] );
  
  /*
  if( kick ) {
    display.fill( WIDTH/4 - wide/2 , WIDTH/4 + wide/2 , pallete[counterColour] );
    display.fill( WIDTH*3/4 - wide/2 , WIDTH*3/4 + wide/2 , pallete[counterColour] );
  }
  */
  
  //Update Km
  updateKm();

  //Shift phase: Kilomux way
  rsd.shiftPhase( map( pot[2] , 0 , 1023 , -1 , +2 ) );
}

// MIDI Input  //////////////////////////////////////////////////////////////////////////

void handleNoteOn( byte channel , byte pitch , byte velocity ) {
  kick = true;
  wide = velocity;
  counterColour = 1 - counterColour;
  led[ 4 + channel ] = HIGH;
}


void handleNoteOff( byte channel , byte pitch , byte velocity ) {
  kick = false;
  led[ 4 + channel ] = LOW;
}

// Update Kilomux ///////////////////////////////////////////////////////////////////////

void updateKm() {
  
  for( int i = 0 ; i < 8 ; i++ ) {
    //Update pots
    pot[i] = KmShield.analogReadKm( MUX_A, i );
  
    //Update buttons
    buttonState[i] = KmShield.digitalReadKm( MUX_B , i , PULLUP );
    
    //Click detection
    if ( ( buttonState[i] == LOW ) && ( buttonLastState[i] == HIGH ) ) {
        buttonPushCounter[i]++;
    }
    
    switch ( i ) {

     //Button 0 = On/Off
      case 0: 
        if ( buttonPushCounter[i]&1 ) {
          led[i] = HIGH;
          rsd.switchOff();
        } else {
          led[i] = LOW;
          rsd.switchOn();
        }
      break;
      
      //Button 1 = Bloq Freq
      case 1: 
        if ( buttonPushCounter[i]&1 ) {
          led[i] = HIGH;
          bloq = true;
        } else {
          led[i] = LOW;
          bloq = false;
        }
      break;
      /*  
      //Button 2 = Background keyboard
      case 2: 
        if ( buttonPushCounter[i]&1 ) {
          led[i] = HIGH;
          pianoKeyboardVisible = true;
        } else {
          led[i] = LOW;
          pianoKeyboardVisible = false;
        }
      break;
   /*
    case 3: //next Button
      if ( !buttonState[i] ) {
        led[i] = HIGH;
        if ( buttonLastState[i] ) {
          if ( screen < screen_size ) {
            screen++;
          } else {
            screen = 0;
          }
        }
      } else {
        led[i] = LOW;
      }
      break;
     break;
    */
    /*
    case( 4 ):
      //Push
      if ( !buttonState[i] && buttonLastState[i] ) {
        MIDI.sendNoteOn( 60 , 127 , 1 );
        notes[0][ 60%24 ] = true;
        led[i] = HIGH;
      }
      //Release
      if( buttonState[i] && !buttonLastState[i] ){
        MIDI.sendNoteOff( 60 , 0 , 1 );
        notes[0][ 60%24 ] = false;
        led[i] = LOW;
      }
      break;
      
    case( 5 ):
      //Push
      if ( !buttonState[i] &&  buttonLastState[i] ) {
        MIDI.sendNoteOn( 60 , 127 , 2 );
        notes[1][ 60%24 ] = true;
        led[i] = HIGH;
      }
      //Release 
      if( buttonState[i] && !buttonLastState[i] ){
        MIDI.sendNoteOff( 60 , 0 , 2 );
        notes[1][ 60%24 ] = false;
        led[i] = LOW;
      }
      break;
    
    case( 6 ):
      //Push
      if ( !buttonState[i] && buttonLastState[i] ) {
        MIDI.sendNoteOn( 60 , 127 , 3 );
        notes[2][ 60%24 ] = true;
        led[i] = HIGH;
      } 
      //Release
      if( buttonState[i] && !buttonLastState[i] ){
        MIDI.sendNoteOff( 60 , 0 , 3 );
        notes[2][ 60%24 ] = false;
        led[i] = LOW;
      }
      break;
    
    /*
    case( 7 ):
      if ( buttonPushCounter[i]&1 ) {
        led[i] = HIGH;
        pianoKeyboardVisible = true;
      } else {
        led[i] = LOW;
        pianoKeyboardVisible = false;
      }
      break;
      */
     default: break;
    }
    
    buttonLastState[i]= buttonState[i];
  }

  
  //Update leds
  for( int i = 0 ; i < 8 ; i++ ) {
    KmShield.digitalWriteKm( i + 8 , led[i] );
  }

}
