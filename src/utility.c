#include "utility.h"
#include <stdlib.h>

List* utilCreateList() {
  List* result = malloc( sizeof( List ) );

  result->data = NULL;
  result->next = NULL;

  return result;
}

void utilCloseList( List* first ) {

  List* current = first;

  while( current != NULL ) {
    free( current->data );

    List* prev = current;
    current = current->next;
    free( prev );
  }

}
