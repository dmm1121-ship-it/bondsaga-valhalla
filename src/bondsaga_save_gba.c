#include "global.h"
#include "bondsaga_save_gba.h"
#include "load_save.h"
#include "gba/flash_internal.h"

static bool Ready(void)
{
    return gFlashMemoryPresent == TRUE && gFlash && gFlash->romSize == BSG_FLASH_BYTES
        && gFlash->sector.size == BSG_SECTOR_BYTES && gFlash->sector.count == 32
        && ProgramFlashByte && EraseFlashSector;
}

static bool ReadBytes(void *context, uint32_t offset, uint8_t *out, uint32_t length)
{
    (void)context;
    if (!Ready() || offset > BSG_FLASH_BYTES || length > BSG_FLASH_BYTES - offset) return false;
    while (length) {
        uint32_t within = offset % BSG_SECTOR_BYTES;
        uint32_t n = BSG_SECTOR_BYTES - within;
        if (n > length) n = length;
        ReadFlash(offset / BSG_SECTOR_BYTES, within, out, n);
        offset += n; out += n; length -= n;
    }
    return true;
}

static bool EraseSector(void *context, uint16_t sector)
{
    (void)context;
    return Ready() && sector < 32 && EraseFlashSector(sector) == 0;
}

static bool ProgramBytes(void *context, uint32_t offset, const uint8_t *bytes, uint32_t length)
{
    (void)context;
    if (!Ready() || offset > BSG_FLASH_BYTES || length > BSG_FLASH_BYTES - offset) return false;
    while (length--) {
        /* ProgramFlashSector erases internally, so cannot be used here or for
         * the final marker. The transaction verifies all skipped erased bytes. */
        if (*bytes != 0xff && ProgramFlashByte(offset / BSG_SECTOR_BYTES, offset % BSG_SECTOR_BYTES, *bytes)) return false;
        bytes++; offset++;
    }
    return true;
}

bool BsgGbaOpenIo(BsgIo *io)
{
    if (!io || !Ready()) return false;
    io->context = NULL;
    io->read = ReadBytes;
    io->erase = EraseSector;
    io->program = ProgramBytes;
    return true;
}

bool BsgGbaLegacyAccessAllowed(void)
{
    BsgIo io;
    if (!BsgGbaOpenIo(&io)) return false;
    /* Check every sector, not only a committed header: losing either first
     * sector must not expose the remaining recovery image to Emerald writes. */
    return BsgLegacyAccessAllowed(&io);
}
