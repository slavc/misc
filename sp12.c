/*
 * A simple software programmer for Atmel ATmega8 and ATTiny microcontrollers
 * which utilizes the LPT port.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <err.h>

#include <amd64/pio.h>

#define NELEMS(a) (sizeof(a)/sizeof(a[0]))

#define SCK_DELAY 1UL

#define LPT_DATA_PORT 0x378
#define LPT_STAT_PORT (LPT_DATA_PORT+1)
#define LPT_CTRL_PORT (LPT_DATA_PORT+2)

#define BIT_0 0x1
#define BIT_1 0x2
#define BIT_2 0x4
#define BIT_3 0x8
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80

#define VCC_BITS (BIT_2|BIT_3|BIT_4|BIT_5|BIT_6)
#define SCK_BIT BIT_0
#define RST_BIT BIT_1
#define MOSI_BIT BIT_7
#define MISO_BIT BIT_7

extern const char *__progname;

int dflag;

u_int8_t data_reg, stat_reg, ctrl_reg;

const char *mcu_models[] = {
    "atmega8",
    "attiny2313",
    //"attiny13", /* couldn't get ATTiny13 to work, so disable it for now */
    NULL
};
enum e_mcu_models {
    ATMEGA8,
    ATTINY2313,
    //ATTINY13,
    N_MCU_MODELS
};
struct mcu_params {
    int model;

    int freq;

    int npages;
    int page_size; /* in bytes */
    int word_size;

    /* these are computed from above ones */
    int flash_size;
    int nwords;
} const mcu_model_params[] = {
    { ATMEGA8,    1000000, 128, 64, 2, 128*64, (128*64)/2 },
    { ATTINY2313, 1000000,  64, 32, 2,  64*32,  (64*32)/2 },
    //{ ATTINY13,   9600000,  32, 32, 2,  32*32,  (32*32)/2 }
};

long sck_half_period; /* in microseconds */

char *sp12_commands[] = {
    "test",
    "fdump",
    "fburn",
    "erase",
    "power_on",
    "power_off",
    "write_fuse_bits",
    NULL
};
enum e_sp12_commands {
    SP12_TEST,
    SP12_DUMP_FLASH,
    SP12_BURN_FLASH,
    SP12_ERASE,
    SP12_POWER_ON,
    SP12_POWER_OFF,
    SP12_WRITE_FUSE_BITS,
    N_COMMANDS
};

#define WRITE_FUSE_BITS_HELP_MSG "Argument for write fuse bits command must be in the\nformat h|l=<hex_val>, for example 'h=f0'."

int
dbgprintf(const char *fmt, ...) {
    va_list ap;
    int n;

    if (!dflag)
        return 0;
    va_start(ap, fmt);
    n = vfprintf(stderr, fmt, ap);
    va_end(ap);

    return n;
}

/* computes stuff like time delays... */
void
compute_values(struct mcu_params *mcu_params) {
    double usec;

    usec = 1.0 / (double) mcu_params->freq * 1000000.0;
    usec *= (double) mcu_params->freq < 12000000.0 ? 3.0 : 4.0;
    if (usec < 1)
        sck_half_period = 2;
    else
        sck_half_period = (long) ceil(usec);
    dbgprintf("sck_half_period = %ld\n", sck_half_period);
}

/* 
 * microsecond delay
 * max 'us' value is 2^31-1000000 (== ~1000000)
 */
void
udelay(long us) {
    struct timeval end, t;
    long usec, sec;

    gettimeofday(&end, NULL);
    sec = end.tv_sec;
    usec = end.tv_usec + us;
    if (usec >= 1000000) {
        usec -= 1000000;
        ++sec;
    }
    end.tv_sec = sec;
    end.tv_usec = usec;

    for ( ;; ) {
        gettimeofday(&t, NULL);
        sec = end.tv_sec - t.tv_sec;
        usec = end.tv_usec - t.tv_usec;
        if (sec <= 0 && usec <= 0)
            break;
    }
}

/*
 * millisecond delay 
 */
void
mdelay(long ms) {
    while (ms--)
        udelay(1000);
}

int
enable_io(void) {
    return amd64_iopl(3);
}

void
power_on(void) {
    data_reg |= VCC_BITS;
    ctrl_reg |= BIT_3;
    outb(LPT_DATA_PORT, data_reg);
    outb(LPT_DATA_PORT, ctrl_reg);
}

void
power_off(void) {
    outb(LPT_DATA_PORT, 0);
    outb(LPT_STAT_PORT, 0 | BIT_7);
    outb(LPT_CTRL_PORT, 0 | BIT_0 | BIT_1 | BIT_3);
}

void
activate_reset(void) {
    data_reg &= ~RST_BIT; /* RESET on mcu is inverted */
    outb(LPT_DATA_PORT, data_reg);
}

void
deactivate_reset(void) {
    data_reg |= RST_BIT;
    outb(LPT_DATA_PORT, data_reg);
}

void
set_sck(void) {
    data_reg |= SCK_BIT;
    outb(LPT_DATA_PORT, data_reg);
}

void
clr_sck(void) {
    data_reg &= ~SCK_BIT;
    outb(LPT_DATA_PORT, data_reg);
}

void
set_mosi(void) {
    data_reg |= MOSI_BIT;
    outb(LPT_DATA_PORT, data_reg);
}

void
clr_mosi(void) {
    data_reg &= ~MOSI_BIT;
    outb(LPT_DATA_PORT, data_reg);
}

/* returns non-zero if miso is 1 */
int
read_miso(void) {
    stat_reg = inb(LPT_STAT_PORT);
    if (stat_reg & MISO_BIT) /* pin11 is inverted */
        return 0;
    else
        return 1;
}

void
chip_restart(void) {
    dbgprintf("chip_restart()\n");
    deactivate_reset();
    mdelay(2);
    activate_reset();
    mdelay(40);
}

int
init_lpt(void) {
    dbgprintf("init_lpt()\n");
    if (enable_io() == -1)
        err(1, "Failed to enable access to I/O ports");

    /* set 0 volts on all LPT pins */
    power_off();

    data_reg = inb(LPT_DATA_PORT);
    stat_reg = inb(LPT_STAT_PORT);
    ctrl_reg = inb(LPT_CTRL_PORT);
}

u_int8_t
byte_io(u_int8_t outbyte) {
    u_int8_t inbyte = 0;
    u_int8_t mask = 1 << 7;
    u_int8_t tmp;
    int i;

    for (i = 0; i < 8; ++i) {
        if (outbyte & mask)
            data_reg |= MOSI_BIT;
        else
            data_reg &= ~MOSI_BIT;
        outb(LPT_DATA_PORT, data_reg);

        udelay(sck_half_period);

        data_reg |= SCK_BIT;
        outb(LPT_DATA_PORT, data_reg);

        udelay(sck_half_period);

        tmp = inb(LPT_STAT_PORT);
        if (!(tmp & MISO_BIT)) /* pin11 is inverted */
            inbyte |= mask;

        data_reg &= ~SCK_BIT;
        outb(LPT_DATA_PORT, data_reg);

        mask >>= 1;
    }

    return inbyte;
}


void
dump_bufs(const char *name, u_int8_t out_buf[4], u_int8_t in_buf[4]) {
    int i;

    if (dflag < 2)
        return;

    if (name)
        printf("%s: ", name);

    printf("out_buf: ");
    for (i = 0; i < 4; ++i)
        printf("0x%02x ", out_buf[i]);

    putchar('\n');

    if (name)
        printf("%s: ", name);

    printf(" in_buf: ");
    for (i = 0; i < 4; ++i) 
        printf("0x%02x ", in_buf[i]);

    putchar('\n');
}


void
exec_instr(u_int8_t out_buf[4], u_int8_t in_buf[4]) {
    int i;

    for (i = 0; i < 4; ++i)
        in_buf[i] = byte_io(out_buf[i]);

    dump_bufs(NULL, out_buf, in_buf);
}

int
prog_enable(void) {
    u_int8_t out_buf[4] = { 0xac, 0x53, 0x0, 0x0 };
    u_int8_t in_buf[4];

    exec_instr(out_buf, in_buf);

    if (in_buf[2] != 0x53)
        return 0;
    return 1;
}

int
dump_flash(struct mcu_params *mcu_params, FILE *fp) {
    int i;
    u_int8_t data[mcu_params->flash_size];
    u_int8_t out_buf[4] = { 0x20, 0x0, 0x0, 0x0 };
    u_int8_t in_buf[4];

    dbgprintf("dump_flash()\n");

    for (i = 0; i < mcu_params->flash_size; ++i) {
        out_buf[0] = i & 1 ? 0x28 : 0x20;
        out_buf[1] = (i >> 9) & 0xf;
        out_buf[2] = (i >> 1) & 0xff;

        exec_instr(out_buf, in_buf);
        data[i] = in_buf[3];
    }

    if (fp)
        fwrite(data, 1, mcu_params->flash_size, fp);

    return 1;
} 

void
burn_firmware(struct mcu_params *mcu_params, u_int8_t *data, int npages) {
    unsigned int page, addr, page_addr;
    unsigned int byte;
    u_int8_t out_buf[4] = { 0x0, 0x0, 0x0, 0x0 };
    u_int8_t in_buf[4];

    dbgprintf("burn_firmware()\n");

    switch (mcu_params->model) {
    case ATMEGA8:
        addr = 0;
        for (page = 0; page < npages; ++page) {
            dbgprintf("writing page: %i\n", page);
            for (byte = 0; byte < mcu_params->page_size; ++byte) {
                out_buf[0] = byte & 1 ? 0x48 : 0x40;
                out_buf[1] = 0;
                out_buf[2] = (byte >> 1) & 0xff;
                out_buf[3] = data[addr++];

                exec_instr(out_buf, in_buf);
            }

            /* write the page */
            page_addr = page * mcu_params->page_size;
            out_buf[0] = 0x4c;
            out_buf[1] = (page_addr >> 9) & 0xf;
            out_buf[2] = (page_addr >> 1) & 0xf0;
            out_buf[3] = 0;

            exec_instr(out_buf, in_buf);

            mdelay(15);
        }
        break;
    case ATTINY2313:
        addr = 0;
        for (page = 0; page < npages; ++page) {
            dbgprintf("writing page: %i\n", page);
            for (byte = 0; byte < mcu_params->page_size; ++byte) {
                out_buf[0] = byte & 1 ? 0x48 : 0x40;
                out_buf[1] = 0;
                out_buf[2] = (byte >> 1) & 0xf;
                out_buf[3] = data[addr++];

                exec_instr(out_buf, in_buf);
            }

            page_addr = page * mcu_params->page_size;
            out_buf[0] = 0x4c;
            out_buf[1] = page_addr >> 9 & 0x3;
            out_buf[2] = page_addr >> 1 & 0xf0;
            out_buf[3] = 0;

            exec_instr(out_buf, in_buf);

            mdelay(8);
        }
        break;
    }
}

void
write_fuse_bits(const char *arg) {
    char which_byte;
    unsigned int value;
    u_int8_t out_buf[4] = { 0xac, 0, 0, 0 };
    u_int8_t in_buf[4];

    if (sscanf(arg, "%c=%X", &which_byte, &value) != 2) {
        warnx("%s", WRITE_FUSE_BITS_HELP_MSG);
        return;
    }

    if (which_byte == 'H' || which_byte == 'h')
        out_buf[1] = 0xa8;
    else
        out_buf[1] = 0xa0;
    out_buf[3] = value;

    exec_instr(out_buf, in_buf);
    mdelay(20);
}

void
chip_erase(void) {
    u_int8_t out_buf[4] = { 0xac, 0x80, 0x0, 0x0 };
    u_int8_t in_buf[4];

    dbgprintf("chip_erase()\n");

    exec_instr(out_buf, in_buf);
    mdelay(30);
}

u_int8_t *
load_file(const char *filename, size_t *buffer_size) {
    FILE *fp;
    u_int8_t *buf = NULL;
    const size_t chunk_size = 8192;
    u_int8_t chunk[8192];
    size_t nbytes = 0;
    size_t buf_size = 0;


    if ((fp = fopen(filename, "rb")) == NULL) {
        *buffer_size = 0;
        return NULL;
    }

    while ((nbytes = fread(chunk, 1, chunk_size, fp)) > 0) {
        buf = realloc(buf, buf_size + nbytes);
        memcpy(buf + buf_size, chunk, nbytes);
        buf_size += nbytes;
    }
    fclose(fp);

    *buffer_size = buf_size;
    return buf;
}

u_int8_t *
load_firmware_file(struct mcu_params *mcu_params, const char *filename, size_t *fw_size) {
    u_int8_t *data;
    size_t data_size;
    size_t i;

    if ((data = load_file(filename, &data_size)) == NULL)
        return NULL;

    if (data_size % mcu_params->page_size) {
        i = data_size;
        data_size += mcu_params->page_size - (data_size % mcu_params->page_size);
        data = realloc(data, data_size);
        while (i < data_size)
            data[i++] = 0xff;
    }

    *fw_size = data_size;
    return data;
} 

void
prog_enable_or_die(void) {
    int tries;
    dbgprintf("prog_enable_or_die()\n");

    for (tries = 0; /* empty */; ++tries) {
        if (tries >= 3) {
            power_off();
            errx(1, "Programming Enable instruction failed");
        }
        if (prog_enable())
            break;
        deactivate_reset();
        udelay(500);
        activate_reset();
    }
}

int
sp12(struct mcu_params *mcu_params, int cmd, const char *argv[]) {
    va_list ap;
    int tries, i;
    FILE *fp;
    const char *filename;
    u_int8_t *buf;
    size_t buf_size;
    const char *arg;

    switch (cmd) {
    case SP12_DUMP_FLASH:
    case SP12_BURN_FLASH:
    case SP12_WRITE_FUSE_BITS:
        /* check that an argument is present */
        if (*argv == NULL)
            errx(1, "An argument must be supplied for this command.");
    };

    init_lpt();

    switch (cmd) {
    case SP12_POWER_OFF:
        /* init_lpt() turns the power off */
        return 0;
        break;
    case SP12_POWER_ON:
        power_on();
        activate_reset();
        mdelay(10);
        deactivate_reset();
        return 0;
        break;
    }

    compute_values(mcu_params);
    power_off();
    clr_mosi();
    clr_sck();
    activate_reset();
    power_on();
    mdelay(40);
    //prog_enable_or_die();

    switch (cmd) {
    case SP12_TEST:
        prog_enable_or_die();
        break;

    case SP12_DUMP_FLASH:
        prog_enable_or_die();
        filename = *argv;

        if ((fp = fopen(filename, "wb")) == NULL)
            err(1, "Failed to open \"%s\" for writing", filename);

        dbgprintf("dumping flash to file \"%s\"\n", filename);
        dump_flash(mcu_params, fp);
        fclose(fp);
        break;

    case SP12_BURN_FLASH:
        prog_enable_or_die();
        filename = *argv;

        if ((buf = load_firmware_file(mcu_params, filename, &buf_size)) == NULL) {
            warn("Failed to open \"%s\"", filename);
            break;
        }

        chip_erase();
        chip_restart();
        prog_enable_or_die();
        burn_firmware(mcu_params, buf, buf_size/mcu_params->page_size);
        free(buf);
        break;
    case SP12_ERASE:
        chip_erase();
        break;
    case SP12_WRITE_FUSE_BITS:
        prog_enable_or_die();
        write_fuse_bits(*argv);
        break;
    }

    power_off();

    return 0;
}

void
usage(void) {
    int i;

    printf("usage: %s [-d] -m <mcu_model> [-f <frequency>] <cmd> [args...]\n"
            "\t-d -- enable debug output\n",
            __progname);
    printf("available commands:\n");
    for (i = 0; i < N_COMMANDS; ++i)
        printf("	%s\n", sp12_commands[i]);

    exit(1);
}

void
signal_handler(int sig) {
    if (sig == SIGHUP)
        return;
    power_off();
    exit(0);
}

void
hexdump(void *data, size_t size) {
    u_int8_t *p = data;
    size_t i;

    if (!p) {
        printf("(null)\n");
        return;
    }

    for (i = 0; i < size; ++i) {
        if (i != 0) {
            if (i % 64 == 0) {
                putchar('\n');
                putchar('\n');
            } else if (i % 16 == 0) {
                putchar('\n');
            } else if (i % 8 == 0) {
                putchar(' ');
                putchar(' '); 
            } else if (i % 4 == 0) {
                putchar(' ');
            }
        }
        printf("%02x", p[i]);
    }

    putchar('\n');
}

int
main(int argc, char *argv[]) {
    int ch;
    int i, j, freq = 0;
    int signals[] = { SIGHUP, SIGINT, SIGTERM };
    const char *arg;

    struct mcu_params mcu_params;

    bzero(&mcu_params, sizeof mcu_params);

    while ((ch = getopt(argc, argv, "dm:f:")) != -1) {
        switch (ch) {
        case 'd':
            ++dflag;
            break;
        case 'm':
            for (i = 0; i < N_MCU_MODELS; ++i)
                if (!strcmp(optarg, mcu_models[i]))
                    break;
            if (i == N_MCU_MODELS) {
                printf("supported mcu models:\n");
                for (i = 0; i < N_MCU_MODELS; ++i)
                    printf("  %s\n", mcu_models[i]);
            }
            mcu_params = mcu_model_params[i];
            break;
        case 'f':
            if (sscanf(optarg, "%u", &freq) != 1)
                usage();
            break;
        default:
            usage();
            break;
        }
    }
    argc -= optind;
    argv += optind;

    if (mcu_params.freq == 0) { /* user didn't specify -m */
        usage();
        exit(1);
    }
    if (freq != 0) /* user has overridden default frequency */
        mcu_params.freq = freq;

    if (argc < 1)
        usage();

    for (i = 0; sp12_commands[i] != NULL; ++i) {
        if (!strcmp(sp12_commands[i], argv[0])) {
            for (j = 0; j < NELEMS(signals); ++j)
                signal(signals[j], signal_handler);

            sp12(&mcu_params, i, argv + 1);

            exit(0);
        }
    }
    usage();

    exit(0);
}
