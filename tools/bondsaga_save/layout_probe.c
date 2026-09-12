/* Compile only, never link into a ROM. Symbol sizes expose target ABI sizes. */
#include "bondsaga_foundation.h"

const unsigned char measure_BsgWorkspace[sizeof(BsgWorkspace)] = {0};
const unsigned char measure_BsgSnapshot[sizeof(BsgSnapshot)] = {0};
const unsigned char measure_BsgImageSpec[sizeof(BsgImageSpec)] = {0};
const unsigned char measure_BsgCreature[sizeof(struct BsgCreature)] = {0};
const unsigned char measure_BsgCreatureStore[sizeof(struct BsgCreatureStore)] = {0};
const unsigned char measure_BsgPlayerBinder[sizeof(struct BsgPlayerBinder)] = {0};
const unsigned char measure_BsgRoster[sizeof(struct BsgRoster)] = {0};
