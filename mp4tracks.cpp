#include <unistd.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <mp4v2/mp4v2.h>

static void usage();
static void info(const char *);
static void remove(const char *, unsigned);
static void optimize(const char *);

/*
 * List/delete tracks in MP4 file.
 */
int main(int argc, char** argv) {
    int ch;
    unsigned i = ~0;
    extern char* optarg;
    extern int optind;

    while ((ch = getopt(argc, argv, "hd:")) != -1) {
        switch (ch) {
        case 'h':
            usage();
            return 0;
        case 'd':
            i = atoi(optarg);
            break;
        default:
            usage();
            return 1;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc == 0) {
        usage();
        return 1;
    }

    while (argc--) {
        if (i == (unsigned)~0) {
            info(*argv);
        } else {
            remove(*argv, i);
            optimize(*argv);
        }
        ++argv;
    }

    return 0;
}

static void usage() {
    std::cout << "usage: mp4tracks -h\n"
              << "       mp4tracks <mp4file>\n"
              << "       mp4tracks -d <index> <mp4file>\n"
              << "\n"
              << "  -d <index>  -- delete track <index>"
              << std::endl;
}

static void info(const char *path) {
    auto h = MP4Modify(path);
    if (h == MP4_INVALID_FILE_HANDLE) {
        std::cerr << path << ": failed to open" << std::endl;
        exit(1);
    }

    auto numTracks = MP4GetNumberOfTracks(h);
    std::cout << path << ": " << numTracks << " tracks" << std::endl;
    for (unsigned i = 0; i < numTracks; ++i) {
        auto t = MP4FindTrackId(h, i);
        std::cout << " " << i << ": ";
        std::cout << MP4GetTrackType(h, t);
        char buf[128] = "";
        if (MP4GetTrackLanguage(h, t, buf)) {
            std::cout << " " << buf;
        }
        std::cout << std::endl;
    }

    MP4Close(h);
}

static void remove(const char *path, unsigned i) {
    auto h = MP4Modify(path);
    if (h == MP4_INVALID_FILE_HANDLE) {
        std::cerr << path << ": failed to open" << std::endl;
        exit(1);
    }

    auto numTracks = MP4GetNumberOfTracks(h);
    if (i >= numTracks) {
        std::cerr << path << ": file has only " << numTracks << " tracks" << std::endl;
    }

    auto t = MP4FindTrackId(h, i);
    if (!MP4DeleteTrack(h, t)) {
        std::cerr << path << ": failed to delete track " << i << std::endl;
        exit(1);
    }

    MP4Close(h);
}

static void optimize(const char *path) {
    std::string tmp;

    tmp = path;
    tmp += ".tmp";

    if (!MP4Optimize(path, tmp.c_str())) {
        std::cerr << path << ": failed to optimize" << std::endl;
        if (unlink(tmp.c_str()) == -1) {
            std::cerr << tmp << ": failed to remove temporary file" << std::endl;
        }
        exit(1);
    }

    if (rename(tmp.c_str(), path) == -1) {
        std::cerr << tmp << ": failed to move temporary file to destination " << path << std::endl;
        exit(1);
    }
}
