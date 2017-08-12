package HelloWorld;

import CharacterDisplay from Scorpion.RetroTK.ExampleConsole;
import String from Scorpion.Language;

u8 function main( String[] arguments ) {

  u8 x = 0;
  u8 y = 0;

  CharacterDisplay display = new CharacterDisplay( CharacterDisplay.Plane.A );
  display.out( "Hello, World!", x, y );

  return 0;

}

/////

package Scorpion.RetroTK.ExampleConsole;

import String from Scorpion.Language;

class CharacterDisplay {

  private {
    const u8 pointer mmioGraphicsControl = 0xB16B00B5;
    const u32 pointer mmioGraphicsData = 0xB16A55E5;
    Plane plane;
  }

  public {

    enum Plane { A, B, WINDOW, SPRITE }

    CharacterDisplay( Plane plane ) {
      this.plane = plane;

      // Write characters to VRAM
      manual {
        *mmioGraphicsControl = 01101011b;

        for( u32 character : CharacterSet ) {
          *mmioGraphicsData = character;
        }
      }
    }

    manual void out( String string, u8 x, u8 y ) {
      for( u8 character : string.toU8Array() ) {
        *mmioGraphicsControl = 10000000b;
        *mmioGraphicsData = ( x << 8 ) | y;
      }
    }

  }

}
