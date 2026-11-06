/* DQK.EXE resident module 1e56: the sound layer over the AIL drivers.
 *
 * EXE 0x010966..0x01213b, one Turbo C large-model _TEXT segment starting at
 * byte 6 of paragraph 1e56.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * Two devices: [0] plays XMIDI music (.XMI) through an .ADV driver, [1]
 * plays digitised sound (.VOC).  A driver file is read into a memory slot,
 * aligned to a paragraph and registered; each call names the data file,
 * whose extension selects the device.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  The 000c callees are a driver API table (6-byte stride); their
 * names follow the AIL 2 functions whose call shapes they match, and are
 * placeholders until that segment is reconstructed.  The rest are named
 * from their call shapes, also placeholders.
 *
 * 19 of the 21 static helpers are reached with `push cs; call near` ahead of
 * their definitions, which Turbo C gives only a static function.  The other
 * two (xmi_install_timbres, timbre_file_name) are called only after theirs,
 * where a public function is called the same way, so the bytes do not force
 * their `static`.  The two callbacks it hands out (timbre_load_cb,
 * file_load_cb) take a far call and a `mov ax,seg`.
 *
 * -d merges the two "ADV" literals into one (DGROUP 0x1e48).  _BSS follows
 * the statics' names (Turbo C's symbol-table order), not their declaration
 * order; these names reproduce the shipped a816..a845 layout.
 */

#include "dqk.h"

struct DriverDesc {                     /* AIL driver description */
    int  min_api;                       /* +00 */
    int  type;                          /* +02 3 = XMIDI, 2 = digital */
    char suffix[4];                     /* +04 timbre file extension */
    void far *names;                    /* +08 */
    int  io;                            /* +0c */
    int  irq;                           /* +0e */
    int  dma;                           /* +10 */
    int  drq;                           /* +12 */
};

struct SoundDev {
    int  kind;                          /* +00 0 = music, 1 = digital */
    int  driver;                        /* +02 driver handle, -1 = none */
    char driver_file[12];               /* +04 */
    char loaded[12];                    /* +10 data file now resident */
    int  count;                         /* +1c sequences or VOC markers */
    int  slot;                          /* +1e memory slot of the driver */
    void far *image;                    /* +20 driver image, para-aligned */
    struct DriverDesc far *desc;        /* +24 */
    char far *data;                     /* +28 the loaded data file */
    int  seq[6];                        /* +2c sequence handles, -1 = none */
    void far *state[6];                 /* +38 sequence state tables */
};

union TimbreKey {
    unsigned w;
    signed char b[2];                   /* patch, bank; bank -1 ends a list */
};

struct TimbreEntry {                    /* one record of the timbre index */
    signed char patch;
    signed char bank;
    long offset;
};

enum {
    EXT_XMI  = 0x4d58,                  /* "XM", the low word of "XMI" */
    EXT_VOC  = 0x4f56,                  /* "VO", the low word of "VOC" */
    DOS_READ = 0x3f00                   /* int 21h AH=3fh */
};

extern void far path_default_ext(char far *path, char far *ext, int force); /* 1c60:025c */
extern void far huge_move(void far *src, void far *dst, long n); /* 00d7:08c2 */

static int driver_load(struct SoundDev far *dev, char far *path);
static int driver_read(struct SoundDev far *dev, char far *path);
static int driver_align(struct SoundDev far *dev);
static int driver_start(struct SoundDev far *dev);
static int voc_init(struct SoundDev far *dev);
static int voc_release(struct SoundDev far *dev);
static int voc_load(struct SoundDev far *dev, char far *path);
static int voc_stop(struct SoundDev far *dev, char wait);
static int voc_count_markers(struct SoundDev far *dev);
static int xmi_init(struct SoundDev far *dev);
static int xmi_release(struct SoundDev far *dev);
static int xmi_alloc_states(struct SoundDev far *dev);
static int xmi_alloc_cache(struct SoundDev far *dev);
static int xmi_install_timbres(struct SoundDev far *dev, unsigned sequence_index, int file_handle);
static int timbre_file_name(struct SoundDev far *dev, char far *out);
static void far *timbre_find(int file_handle, char far *key);
static int xmi_load(struct SoundDev far *dev, char far *path);
static int xmi_stop(struct SoundDev far *dev, char wait);
static int xmi_register(struct SoundDev far *dev);
char far timbre_load_cb(int file_handle);
static int file_load(char far *path, void far *buf, long limit);
char far file_load_cb(int file_handle);
static void far *slot_alloc(long byte_count, int slot);

static int driver_type[2] = { 3, 2 };                            /* 1d48 */
static struct SoundDev devices[2] = { 0 };                       /* 1d4c */
static long data_ext[2] = { 0x00494d58L, 0x00434f56L };         /* 1dec "XMI", "VOC" */
static void (far *timer_fn)(void) = timer_tick;                  /* 1df4 */

static unsigned tlen;                                            /* a816 */
static struct SoundDev far *tdev;                                /* a818 */
static int timer_id;                                             /* a81c */
static void far *load_buf;                                       /* a81e */
static unsigned timbre_buf[16];                                  /* a822 */
static long load_limit;                                          /* a842 */

int far sound_init(char far *music_drv, char far *voc_drv)       /* 1e56:0006 */
{
    short started;
    struct SoundDev far *dev;
    register int return_code;
    register int missing;

    return_code = 0;
    started = 0;
    missing = 0;
    if (timer_fn)
        timer_unhook();
    ail_startup();
    if (timer_fn) {
        timer_id = ail_register_timer(timer_fn);
        ail_set_timer_divisor(timer_id, 0x4000);
        ail_start_timer(timer_id);
    }
    if (music_drv && *music_drv) {
        path_default_ext(music_drv, "ADV", 1);
        dev = &devices[0];
        if (dev->image == 0) {
            dev->kind = 0;
            if ((return_code = driver_load(dev, music_drv)) == 0) {
                return_code = xmi_init(dev);
                started += return_code == 0 ? 1 : 0;
            } else if (return_code == -2)
                missing = 1;
        }
    }
    if (!return_code || missing) {
        if (voc_drv && *voc_drv) {
            path_default_ext(voc_drv, "ADV", 1);
            dev = &devices[1];
            if (dev->image == 0) {
                dev->kind = 1;
                if ((return_code = driver_load(dev, voc_drv)) == 0) {
                    return_code = voc_init(dev);
                    started += return_code == 0 ? 1 : 0;
                } else if (return_code == -2)
                    missing += 2;
            }
        }
    }
    return missing ? -missing : started;
}

void far sound_shutdown(void)                                    /* 1e56:017d */
{
    struct SoundDev far *dev;
    register int index;

    for (index = 0; index < 2; index++) {
        dev = &devices[index];
        if (dev->image) {
            switch (dev->kind) {
            case 0:
                if (dev->loaded[0])
                    xmi_stop(dev, 0);
                xmi_release(dev);
                break;
            case 1:
                if (dev->loaded[0])
                    voc_stop(dev, 0);
                voc_release(dev);
                break;
            }
            dev->data = 0;
        }
    }
    if (timer_fn)
        ail_release_timer_handle(timer_id);
    ail_shutdown(0);
    for (index = 0; index < 2; index++)
        slot_release(dev->driver_file); /* dev stays devices[1] (1e56:023a) */
    if (timer_fn)
        timer_rehook();
}

int far sound_play(char far *file, register int index, int volume) /* 1e56:0265 */
{
    short device_index;
    long ext;
    struct SoundDev far *dev;
    char ext_buf[6];
    char name[12];
    register int return_code;

    return_code = 0;
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    path_name(file, name);
    switch ((int)ext) {
    case EXT_XMI:
        if (str_cmp(name, dev->loaded)) {
            if (dev->loaded[0])
                xmi_stop(dev, 0);
            dev->count = int_min(xmi_load(dev, file), 6);
            if (dev->count > 0)
                str_copy(dev->loaded, name);
            else
                return_code = -3;
        }
        if (!return_code && index >= 0 && dev->count > index && dev->seq[index] != -1) {
            ail_set_relative_volume(dev->driver, dev->seq[index], volume, 0);
            ail_start_sequence(dev->driver, dev->seq[index]);
        } else if (!return_code)
            return_code = -5;
        break;
    case EXT_VOC:
        if (str_cmp(name, dev->loaded)) {
            if (dev->loaded[0])
                voc_stop(dev, 0);
            dev->count = int_min(voc_load(dev, file), 0x7ffe);
            if (dev->count > 0)
                str_copy(dev->loaded, name);
            else
                return_code = -3;
        }
        if (!return_code && index >= 0 && dev->count > index) {
            ail_play_voc_file(dev->driver, dev->data, index);
            ail_set_digital_playback_volume(dev->driver, volume);
            ail_start_digital_playback(dev->driver);
        } else if (!return_code)
            return_code = -5;
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

int far sound_volume(char far *file, int index, int volume)      /* 1e56:04cb */
{
    long ext;
    struct SoundDev far *dev;
    char ext_buf[6];
    char name[12];
    register int return_code;
    register int device_index;

    return_code = 0;
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    path_name(file, name);
    switch ((int)ext) {
    case EXT_XMI:
        if (str_cmp(name, dev->loaded))
            break;
        if (!return_code && index >= 0 && dev->count > index && dev->seq[index] != -1)
            ail_set_relative_volume(dev->driver, dev->seq[index], volume, 0);
        else if (!return_code)
            return_code = -5;
        break;
    case EXT_VOC:
        if (str_cmp(name, dev->loaded))
            break;
        if (!return_code && index >= 0 && dev->count > index)
            ail_set_digital_playback_volume(dev->driver, volume);
        else if (!return_code)
            return_code = -5;
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

int far sound_wait(char far *file, int index, char interruptible) /* 1e56:0620 */
{
    long ext;
    short seq;
    struct SoundDev far *dev;
    char name[12];
    char ext_buf[6];
    register int device_index;
    register int return_code;

    return_code = 0;
    path_name(file, name);
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    if (str_cmp(name, dev->loaded))
        return -3;
    if (index < 0 || dev->count <= index)
        return -5;
    switch ((int)ext) {
    case EXT_XMI:
        seq = dev->seq[index];
        if (interruptible) {
            while (ail_sequence_status(dev->driver, seq) == 1)
                if (key_pressed()) {
                    ail_stop_sequence(dev->driver, seq);
                    key_flush();
                    break;
                }
        } else
            while (ail_sequence_status(dev->driver, seq) == 1)
                ;
        break;
    case EXT_VOC:
        if (interruptible) {
            while (ail_voc_playback_status(dev->driver) == 2)
                if (key_pressed()) {
                    ail_stop_digital_playback(dev->driver);
                    key_flush();
                    break;
                }
        } else
            while (ail_voc_playback_status(dev->driver) == 2)
                ;
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

int far sound_stop(char far *file, int index)                    /* 1e56:07ab */
{
    long ext;
    short seq;
    struct SoundDev far *dev;
    char name[12];
    char ext_buf[6];
    register int device_index;
    register int return_code;

    return_code = 0;
    path_name(file, name);
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    if (str_cmp(name, dev->loaded))
        return -3;
    if (index < 0 || dev->count <= index)
        return -5;
    switch ((int)ext) {
    case EXT_XMI:
        seq = dev->seq[index];
        if (ail_sequence_status(dev->driver, seq) == 1)
            ail_stop_sequence(dev->driver, seq);
        break;
    case EXT_VOC:
        if (ail_voc_playback_status(dev->driver) == 2)
            ail_stop_digital_playback(dev->driver);
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

int far sound_pause(char far *file, register int index)          /* 1e56:08db */
{
    short device_index;
    long ext;
    struct SoundDev far *dev;
    char ext_buf[6];
    char name[12];
    register int return_code;

    return_code = 0;
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    path_name(file, name);
    switch ((int)ext) {
    case EXT_XMI:
        if (!return_code && index >= 0 && dev->count > index && dev->seq[index] != -1) {
            if (ail_sequence_status(dev->driver, dev->seq[index]) == 1)
                ail_stop_sequence(dev->driver, dev->seq[index]);
        } else if (!return_code)
            return_code = -5;
        break;
    case EXT_VOC:
        if (!return_code && index >= 0 && dev->count > index)
            ail_pause_digital_playback(dev->driver);
        else if (!return_code)
            return_code = -5;
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

int far sound_resume(char far *file, int index)                  /* 1e56:0a0b */
{
    long ext;
    struct SoundDev far *dev;
    char ext_buf[6];
    char name[12];
    register int return_code;
    register int device_index;

    return_code = 0;
    path_ext(file, ext_buf);
    ext = *(long *)ext_buf;
    for (device_index = 0; device_index < 2; device_index++)
        if (data_ext[device_index] == ext)
            break;
    if (device_index >= 2)
        return -10;
    if ((dev = &devices[device_index]) == 0)
        return 0;
    path_name(file, name);
    switch ((int)ext) {
    case EXT_XMI:
        if (str_cmp(name, dev->loaded)) {
            return_code = -6;
            break;
        }
        if (!return_code && index >= 0 && dev->count > index && dev->seq[index] != -1)
            ail_resume_sequence(dev->driver, dev->seq[index]);
        else if (!return_code)
            return_code = -5;
        break;
    case EXT_VOC:
        if (str_cmp(name, dev->loaded)) {
            return_code = -6;
            break;
        }
        if (!return_code && index >= 0 && dev->count > index)
            ail_resume_digital_playback(dev->driver);
        else if (!return_code)
            return_code = -5;
        break;
    default:
        return_code = -10;
    }
    return return_code;
}

static int driver_load(struct SoundDev far *dev, char far *path) /* 1e56:0b54 */
{
    register int return_code;

    return_code = 0;
    dev->driver = -1;
    switch (dev->kind) {
    case 0:
        dev->slot = 0x16;
        break;
    case 1:
        dev->slot = 0x17;
        break;
    default:
        return_code = -1;
    }
    if ((return_code = driver_read(dev, path)) == 0)
        return_code = driver_start(dev);
    return return_code;
}

static int driver_read(struct SoundDev far *dev, char far *path) /* 1e56:0bb8 */
{
    register int return_code;

    return_code = 0;
    path_name(path, dev->driver_file);
    if (!slot_open(dev->driver_file, dev->slot)) {
        if ((return_code = file_load(path, slot_base(dev->slot), -1L)) == 0) {
            if (slot_alloc(load_limit - slot_used(dev->slot), dev->slot))
                return_code = driver_align(dev);
            else
                return_code = -9;
        } else
            return_code = -3;
    } else
        return_code = -7;
    return return_code;
}

static int driver_align(struct SoundDev far *dev)                /* 1e56:0c6e */
{
    long pad;
    long used;
    char far *base;
    char far *image;
    register int return_code;

    return_code = 0;
    image = base = slot_base(dev->slot);
    if ((pad = (long)image & 0xf) != 0) {
        pad = 16 - pad;
        if (slot_reserve(dev->slot, pad)) {
            used = slot_used(dev->slot);
            image = huge_add(image, pad);
            huge_move(base, image, used);
        } else
            return_code = -9;
    }
    if (!return_code)
        dev->image = image;
    return return_code;
}

static int driver_start(struct SoundDev far *dev)                /* 1e56:0d2b */
{
    struct DriverDesc far *desc;
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    dev->driver = ail_register_driver(dev->image);
    if (dev->driver == -1)
        return_code = 0;                         /* and desc is stored unset below */
    else {
        desc = ail_describe_driver(dev->driver);
        /* v1.0: no RB/SB/SI command-line port and IRQ overrides, and no retry
         * of a Sound Blaster at IRQ 5 when IRQ 7 is not detected */
        if (desc->type != driver_type[dev->kind])
            return_code = -7;
        else if (!ail_detect_device(dev->driver, desc->io, desc->irq, desc->dma, desc->drq))
            return_code = -2;
        else
            ail_init_driver(dev->driver, desc->io, desc->irq, desc->dma, desc->drq);
    }
    if (!return_code)
        dev->desc = desc;
    return return_code;
}

static int voc_init(struct SoundDev far *dev)                    /* 1e56:0ed0 */
{
    register int return_code;

    return_code = 0;
    if (dev == 0 || dev->kind != 1)
        return -7;
    dev->data = slot_alloc(0x78beL, dev->slot);
    if (dev->data == 0)
        return_code = -9;
    return return_code;
}

static int voc_release(struct SoundDev far *dev)                 /* 1e56:0f1e */
{
    register int return_code;

    return_code = 0;
    if (dev == 0 || dev->kind != 1)
        return_code = -7;
    return return_code;
}

static int voc_load(struct SoundDev far *dev, char far *path)    /* 1e56:0f3d */
{
    short markers;

    markers = 0;
    if (dev->image)
        if (file_load(path, dev->data, 0x78beL) == 0)
            markers = voc_count_markers(dev);
    return markers;
}

static int voc_stop(struct SoundDev far *dev, char wait)         /* 1e56:0f8b */
{
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    if (wait)
        while (ail_voc_playback_status(dev->driver) == 2)
            ;
    ail_stop_digital_playback(dev->driver);
    return return_code;
}

static int voc_count_markers(struct SoundDev far *dev)           /* 1e56:0fc8 */
{
    unsigned short markers;
    char far *block_cursor;
    long len;

    markers = 0;
    if (dev->data == 0)
        return -8;
    block_cursor = huge_add(dev->data, 0x14L);     /* the header's data offset */
    len = *(int far *)block_cursor;
    block_cursor = huge_add(dev->data, len);
    while (*block_cursor != 0 && markers < 0x7ffe) {
        if (*block_cursor == 4)                    /* VOC block type 4: marker */
            markers++;
        block_cursor = huge_add(block_cursor, 1L);
        len = *(long far *)block_cursor & 0xffffffL;
        block_cursor = huge_add(block_cursor, len + 3);
    }
    return markers;
}

static int xmi_init(struct SoundDev far *dev)                    /* 1e56:109f */
{
    register int return_code;
    register int index;

    return_code = 0;
    if (dev == 0 || dev->kind != 0)
        return -7;
    for (index = 0; index < 6; index++) {
        dev->seq[index] = -1;
        dev->state[index] = 0;
    }
    dev->data = slot_alloc(0x2148L, dev->slot);
    if (dev->data == 0)
        return_code = -9;
    if (!return_code && (return_code = xmi_alloc_states(dev)) == 0)
        return_code = xmi_alloc_cache(dev);
    return return_code;
}

static int xmi_release(struct SoundDev far *dev)                 /* 1e56:1145 */
{
    register int return_code;

    return_code = 0;
    dev = dev;                          /* the lone les bx at 1e56:114b */
    return return_code;
}

static int xmi_alloc_states(struct SoundDev far *dev)            /* 1e56:1153 */
{
    long size;
    register int index;
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    size = ail_state_table_size(dev->driver);
    for (index = 0; index < 6; index++)
        if ((dev->state[index] = slot_alloc(size, dev->slot)) == 0) {
            return_code = -9;
            break;
        }
    return return_code;
}

static int xmi_alloc_cache(struct SoundDev far *dev)             /* 1e56:11c7 */
{
    void far *cache;
    register int return_code;
    register unsigned size;

    return_code = 0;
    if (dev == 0)
        return -7;
    if ((size = ail_default_timbre_cache_size(dev->driver)) != 0) {
        cache = slot_alloc((long)size, dev->slot);
        if (cache)
            ail_define_timbre_cache(dev->driver, cache, size);
        else
            return_code = -9;
    }
    return return_code;
}

static int xmi_install_timbres(struct SoundDev far *dev, unsigned sequence_index, int file_handle) /* 1e56:1232 */
{
    unsigned short request;
    void far *timbre;
    union TimbreKey key;
    register int return_code;
    register int seq;

    return_code = 0;
    if (dev == 0 || sequence_index < 0 || sequence_index >= 6)
        return 0;
    seq = dev->seq[sequence_index];
    key.w = request = ail_timbre_request(dev->driver, seq);
    while (!return_code && key.b[1] != -1) {
        if ((timbre = timbre_find(file_handle, key.b)) != 0) {
            ail_install_timbre(dev->driver, key.b[1], key.b[0], timbre);
            key.w = request = ail_timbre_request(dev->driver, seq);
        } else
            return_code = -4;
    }
    return return_code;
}

static int timbre_file_name(struct SoundDev far *dev, char far *out) /* 1e56:12c5 */
{
    char far *filename_cursor;
    register int index;
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    str_copy(out, "INSTR");
    for (filename_cursor = out; *filename_cursor; filename_cursor++)
        ;
    index = 0;
    *filename_cursor++ = '.';
    for (; index < 4; index++, filename_cursor++)
        if ((*filename_cursor = dev->desc->suffix[index]) == 0)
            break;
    if (*filename_cursor)
        *filename_cursor = 0;
    return return_code;
}

static void far *timbre_find(register int file_handle, char far *key)     /* 1e56:1346 */
{
    unsigned far *timbre;
    struct TimbreEntry entry;
    register int got;

    if (file_handle < 0)
        return 0;
    if (file_seek(file_handle, 0L, 0) < 0)
        return 0;
    do {
        dos_io(DOS_READ, &entry, file_handle, 6);
        if (entry.bank == -1)
            return 0;
    } while (key[1] != entry.bank || key[0] != entry.patch);
    if (file_seek(file_handle, entry.offset, 0) < 0)
        return 0;
    if ((got = dos_io(DOS_READ, &tlen, file_handle, 2)) != 2 || tlen > 0x20)
        return 0;
    timbre = timbre_buf;
    *timbre = tlen;
    if ((got = dos_io(DOS_READ, timbre + 1, file_handle, tlen - 2)) != tlen - 2)
        return 0;
    return timbre;
}

static int xmi_load(struct SoundDev far *dev, char far *path)    /* 1e56:1437 */
{
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    if (file_load(path, dev->data, 0x2148L) == 0)
        return_code = xmi_register(dev);
    return return_code;
}

static int xmi_stop(struct SoundDev far *dev, char wait)         /* 1e56:1480 */
{
    register int index;
    register int return_code;

    return_code = 0;
    if (dev == 0)
        return -7;
    for (index = 0; index < 6; index++) {
        if (dev->seq[index] == -1)
            break;
        if (wait)
            while (ail_sequence_status(dev->driver, dev->seq[index]) == 1)
                ;
        ail_stop_sequence(dev->driver, dev->seq[index]);
        ail_release_sequence_handle(dev->driver, dev->seq[index]);
    }
    return return_code;
}

static int xmi_register(struct SoundDev far *dev)                /* 1e56:1511 */
{
    short return_code = 0;
    char timbre_file[80] = "";
    register unsigned index;
    register int sequence_index;

    if (dev == 0)
        return -7;
    if (dev->data == 0) {
        return_code = -8;
        sequence_index = 0;
    } else {
        for (index = 0; !return_code && index < 6; index++) {
            dev->seq[index] = ail_register_sequence(dev->driver, dev->data, index,
                                                dev->state[index], 0);
            if (dev->seq[index] == -1)
                break;
        }
        sequence_index = index;
        if (sequence_index > 0 && timbre_file_name(dev, timbre_file) == 0) {
            tdev = dev;
            if (!archive_open(0xb1, timbre_file, 0, timbre_load_cb))
                if (!timbre_load_cb(-1))
                    file_open(0x31, timbre_file, 0, timbre_load_cb);
        }
    }
    return sequence_index;
}

char far timbre_load_cb(int file_handle)                                  /* 1e56:162b */
{
    register unsigned index;
    register int return_code;

    index = return_code = 0;
    for (; !return_code && index < 6; index++) {
        if (tdev->seq[index] == -1)
            break;
        return_code = xmi_install_timbres(tdev, index, file_handle);
    }
    return return_code == 0;
}

static int file_load(char far *path, void far *buf, long limit)  /* 1e56:1675 */
{
    register int return_code;

    return_code = 0;
    load_limit = limit;
    load_buf = buf;
    file_open(0x31, path, 0, file_load_cb);
    return return_code;
}

char far file_load_cb(int file_handle)                                    /* 1e56:16b4 */
{
    long got;
    long want;
    char ok;

    ok = 0;
    want = load_limit >= 0 ? load_limit : 0x7fffffffL;
    got = file_read(file_handle, load_buf, want);
    if (got > 0 && got <= load_limit)
        ok = 1;
    else if (got > 0 && load_limit < 0) {
        load_limit = got;
        ok = 1;
    }
    return ok;
}

static void far *slot_alloc(long byte_count, register int slot)           /* 1e56:1752 */
{
    char far *allocated_data;
    long used;

    if (byte_count < 0)
        byte_count = -byte_count;
    allocated_data = slot_base(slot);
    used = slot_used(slot);
    if (slot_reserve(slot, byte_count))
        allocated_data = huge_add(allocated_data, used);
    else
        allocated_data = 0;
    return allocated_data;
}

/* v1.0 ends the module with four debugging routines v1.1 dropped; nothing in
 * the shipped image calls them.  slot_unreserve hands byte_count bytes back
 * to a slot; the three dumps print a device to the console. */

int far slot_unreserve(long byte_count, int slot)              /* v1.0 only */
{
    register int return_code;

    return_code = 0;
    if (slot_used(slot) >= byte_count)
        slot_reserve(slot, -byte_count);
    else
        return_code = -1;
    return return_code;
}

void far seq_dump(int far *seq)                                 /* v1.0 only */
{
    register int index;

    con_printf("hseq:");
    for (index = 0; index < 6; index++)
        con_printf(" %d", seq[index]);
    con_printf("\n");
}

void far state_dump(void far * far *state)                      /* v1.0 only */
{
    register int index;

    con_printf("state table:");
    for (index = 0; index < 6; index++)
        con_printf(" %08lx", state[index]);
    con_printf("\n");
}

void far dev_dump(struct SoundDev far *dev)                     /* v1.0 only */
{
    con_printf("id: typ %u, dID %d, gNm %s, gID %d, drvr %08lx, dscP %08lx, fil %08lx ",
               dev->kind, dev->driver, dev->driver_file, dev->slot, dev->image,
               dev->desc, dev->data);
    if (dev->kind == 0) {
        seq_dump(dev->seq);
        state_dump(dev->state);
    }
}
