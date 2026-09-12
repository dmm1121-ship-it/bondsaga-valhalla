#ifndef BONDSAGA_SAVE_GBA_H
#define BONDSAGA_SAVE_GBA_H

#include "bondsaga_save.h"

/* Requires the engine's flash identification/timer initialization. The caller
 * supplies frozen, out-of-battle foundation state and workspace; no Emerald
 * save-block conversion or gameplay initializer is implied. */
bool BsgGbaOpenIo(BsgIo *io);
/* Fail closed for a recognized Bondsaga container, including torn snapshots.
 * Protects imported battery saves from legacy loader/recovery/write paths. */
bool BsgGbaLegacyAccessAllowed(void);

#endif
