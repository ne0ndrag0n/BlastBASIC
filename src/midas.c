#include "midas.h"
#include <stdlib.h>

/**
 * Take an abstract syntax tree and create the Type Descriptor Table, the Field Descriptor Table, and the string constants
 */
MidasVM* gsGetMidasVM() {
  MidasVM* vm = calloc( 1, sizeof( MidasVM ) );

  return vm;
}
