uint32 pointerTest( addr offset ) {
  addr original = 0xDEADBEEF;

  uint32 result = *( original * offset );
  return result;
}
