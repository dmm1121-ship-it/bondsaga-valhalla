/* Headless mGBA 0.10 test aid: real frame execution and button input.
 * Build against libmgba's matching headers. No ROM memory writes or save
 * imports are performed. Commands: run <frames> <keys>, shot <path>,
 * read <hex-address> <bytes>, quit. Screenshots are direct framebuffer output.
 */
// mGBA's POSIX directory interface requires PATH_MAX with strict C11/glibc.
#define _POSIX_C_SOURCE 200809L
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <png.h>

static void QuietLog(struct mLogger *logger, int category, enum mLogLevel level, const char *format, va_list args)
{
    if (level == mLOG_FATAL) { vfprintf(stderr, format, args); fputc('\n', stderr); }
}

int main(int argc, char **argv)
{
    char line[512], path[480];
    unsigned frames, keys, address, size;
    color_t pixels[240 * 160];
    struct mLogger logger = { .log = QuietLog };
    struct mCore *core;
    mLogSetDefaultLogger(&logger);
    if (argc != 2 || !(core = mCoreFind(argv[1])) || !core->init(core)) return 1;
    mCoreInitConfig(core, NULL);
    mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "detect");
    mCoreConfigSetDefaultIntValue(&core->config, "logLevel", 0);
    mCoreLoadConfig(core);
    core->setVideoBuffer(core, pixels, 240);
    if (!mCoreLoadFile(core, argv[1])) return 2;
    core->reset(core);
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("READY");
    while (fgets(line, sizeof(line), stdin))
    {
        if (sscanf(line, "run %u %u", &frames, &keys) == 2)
        {
            core->setKeys(core, keys);
            for (unsigned i = 0; i < frames; i++) core->runFrame(core);
            puts("OK");
        }
        else if (sscanf(line, "shot %479s", path) == 1)
        {
            png_image image = { .version = PNG_IMAGE_VERSION, .width = 240, .height = 160, .format = PNG_FORMAT_RGBA };
            puts(png_image_write_to_file(&image, path, 0, pixels, 0, NULL) ? "OK" : "ERROR");
        }
        else if (sscanf(line, "read %x %u", &address, &size) == 2 && size <= 4096)
        {
            for (unsigned i = 0; i < size; i++) printf("%02x", core->busRead8(core, address + i));
            puts("");
        }
        else if (!strncmp(line, "quit", 4)) break;
        else puts("ERROR");
    }
    core->deinit(core);
    return 0;
}
