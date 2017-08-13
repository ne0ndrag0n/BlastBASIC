#include "utility.h"
#include <stdlib.h>

List* utilCreateList() {
  List* result = malloc( sizeof( List ) );

  result->data = NULL;
  result->next = NULL;

  return result;
}
