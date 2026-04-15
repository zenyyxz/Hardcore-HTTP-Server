#include "syscalls.h"
#include "string_utils.h"
#include "assets.h"

// config globals
int g_port = 8080;
const char* g_root = ".";
const char* g_assets = "assets";
uint32_t g_bind = 0; // INADDR_ANY
bool g_quiet = false;
bool g_nozip = false;

// quick check for file extensions
bool has_extension(const char* s, const char* ext) {
    size_t slen = strlen(s);
    size_t elen = strlen(ext);
    if (elen > slen) return false;
    return strcmp(s + slen - elen, ext) == 0;
}

const char* resolve_icon(const char* name, int type) {
    if (type == DT_DIR) return "assets/folder.svg";
    
    // images
    if (has_extension(name, ".png") || has_extension(name, ".jpg") || 
        has_extension(name, ".jpeg") || has_extension(name, ".gif")) return "assets/photo.svg";
    
    // video/audio
    if (has_extension(name, ".mp4") || has_extension(name, ".mkv") || has_extension(name, ".avi")) return "assets/video.svg";
    if (has_extension(name, ".mp3") || has_extension(name, ".wav") || has_extension(name, ".flac")) return "assets/audio.svg";
    
    return "assets/file.svg";
}

// Simple fixed-size path builder
void build_path(char* out, const char* base, const char* name) {
    size_t blen = strlen(base);
    memcpy(out, base, blen);
    if (blen > 0 && out[blen-1] != '/' && name[0] != '/') {
        out[blen++] = '/';
    }
    memcpy(out + blen, name, strlen(name) + 1);
}

// ZIP Central Directory Buffer (fixed size for zero-dependency)
struct zip_cd_state {
    char buffer[16384]; 
    uint32_t offset;
    uint16_t count;
};

void add_to_zip(int client, const char* real_path, const char* zip_path, zip_cd_state* cd, uint32_t* current_offset) {
    struct stat st;
    if (sys_fstat(sys_open(real_path, O_RDONLY), &st) != 0) return;

    if (S_ISDIR(st.st_mode)) {
        int dfd = sys_open(real_path, 0);
        if (dfd < 0) return;
        char buf[2048];
        int n;
        while ((n = sys_getdents64(dfd, (struct linux_dirent64*)buf, sizeof(buf))) > 0) {
            for (int pos = 0; pos < n; ) {
                struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
                if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
                    char next_real[512], next_zip[512];
                    build_path(next_real, real_path, d->d_name);
                    build_path(next_zip, zip_path, d->d_name);
                    add_to_zip(client, next_real, next_zip, cd, current_offset);
                }
                pos += d->d_reclen;
            }
        }
        sys_close(dfd);
    } else {
        int fd = sys_open(real_path, O_RDONLY);
        if (fd < 0) return;

        uint32_t file_crc = 0;
        char fbuf[65536];
        ssize_t rn;
        while ((rn = sys_read(fd, fbuf, sizeof(fbuf))) > 0) {
            file_crc = crc32(fbuf, rn, file_crc);
        }
        sys_close(fd);

        uint16_t path_len = (uint16_t)strlen(zip_path);
        
        // Local header
        zip_local_header lh = {0};
        lh.signature = 0x04034b50;
        lh.version = 20;
        lh.crc32 = file_crc;
        lh.compressed_size = (uint32_t)st.st_size;
        lh.uncompressed_size = (uint32_t)st.st_size;
        lh.filename_len = path_len;
        
        sys_write(client, &lh, sizeof(lh));
        sys_write(client, zip_path, path_len);
        
        fd = sys_open(real_path, O_RDONLY);
        while ((rn = sys_read(fd, fbuf, sizeof(fbuf))) > 0) {
            sys_write(client, fbuf, rn);
        }
        sys_close(fd);

        // Central Directory Header
        zip_central_header ch = {0};
        ch.signature = 0x02014b50;
        ch.version_made = 20;
        ch.version_needed = 20;
        ch.crc32 = file_crc;
        ch.compressed_size = (uint32_t)st.st_size;
        ch.uncompressed_size = (uint32_t)st.st_size;
        ch.filename_len = path_len;
        ch.local_header_offset = *current_offset;

        if (cd->offset + sizeof(ch) + path_len < sizeof(cd->buffer)) {
            memcpy(cd->buffer + cd->offset, &ch, sizeof(ch));
            cd->offset += sizeof(ch);
            memcpy(cd->buffer + cd->offset, zip_path, path_len);
            cd->offset += path_len;
            cd->count++;
        }

        *current_offset += sizeof(lh) + path_len + (uint32_t)st.st_size;
    }
}

void stream_zip(int client, const char* path, const char* name) {
    const char* h_ok = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/zip\r\n"
                       "Content-Disposition: attachment; filename=\"";
    sys_write(client, h_ok, strlen(h_ok));
    sys_write(client, name, strlen(name));
    sys_write(client, ".zip\"\r\n\r\n", 9);

    zip_cd_state cd = {0};
    uint32_t current_offset = 0;
    add_to_zip(client, path, name, &cd, &current_offset);

    // End of Central Directory
    sys_write(client, cd.buffer, cd.offset);

    zip_eocd eocd = {0};
    eocd.signature = 0x06054b50;
    eocd.cd_records_disk = cd.count;
    eocd.cd_records_total = cd.count;
    eocd.cd_size = cd.offset;
    eocd.cd_offset = current_offset;
    
    sys_write(client, &eocd, sizeof(eocd));
}

void render_dir(int client, const char* path, const char* display) {
    const char* top = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n"
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><style>"
        "body { font-family: 'Inter', -apple-system, sans-serif; background: #050505; color: #a9b1d6; max-width: 900px; margin: 40px auto; padding: 0 20px; line-height: 1.5; }"
        "h1 { display: flex; align-items: center; justify-content: space-between; border-bottom: 1px solid #1a1b26; padding-bottom: 15px; margin-bottom: 20px; font-weight: 500; color: #7aa2f7; width: 100%; }"
        "h1 span { display: flex; align-items: baseline; gap: 8px; }"
        ".nav-bar { display: flex; align-items: stretch; gap: 10px; margin-bottom: 30px; }"
        ".path { background: #1a1b26; border: 1px solid #24283b; border-radius: 18px; padding: 0 20px; flex: 1; font-size: 0.85em; display: flex; align-items: center; overflow: hidden; height: 36px; }"
        ".path span { color: #cfc9c2; opacity: 0.9; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }"
        ".controls { display: flex; gap: 6px; }"
        "ul { list-style: none; padding: 0; margin: 0; }"
        "li { padding: 10px 0; border-bottom: 1px solid #1a1b26; display: flex; align-items: center; transition: 0.1s; }"
        "li:hover { background: #16161e; }"
        "a { color: #7aa2f7; text-decoration: none; }"
        "a:hover { color: #89ddff; text-decoration: underline; }"
        "li > a { display: flex; align-items: center; }"
        "li > a:first-child { flex: 1; }"
        ".ico { width: 18px; height: 18px; margin-right: 15px; display: block; filter: invert(0.8); }"
        ".gh-link { display: flex; align-items: center; justify-content: center; background: #fff; color: #000; border-radius: 50%; width: 32px; height: 32px; transition: 0.2s; flex: none; }"
        ".gh-link:hover { transform: scale(1.1); background: #eee; }"
        ".gh-link svg { fill: currentColor; width: 20px; height: 20px; display: block; }"
        ".btn { border-radius: 4px; background: #1f2335; border: 1px solid #292e42; color: #73daca; cursor: pointer; text-decoration: none; display: flex; justify-content: center; align-items: center; transition: all 0.2s; font-size: 0.85em; font-weight: 500; }"
        ".btn:hover { background: #24283b; border-color: #414868; }"
        ".btn-nav { width: 36px; height: 36px; font-size: 1.4em; border-radius: 18px; flex: none; display: flex !important; }"
        ".btn-nav:hover { transform: scale(1.1); color: #89ddff; }"
        ".btn-dl { padding: 6px 0; width: 78px; margin-left: 10px; opacity: 0.7; font-size: 0.75em; flex: none; display: flex !important; }"
        ".btn-dl:hover { opacity: 1; }"
        "</style></head><body>"
        "<h1><span>RawServe <small style='font-size:0.6em;opacity:0.4'>v0.2</small></span>"
        "<a href=\"https://github.com/zenyyxz/http_server\" class=\"gh-link\">"
        "<svg viewBox=\"0 0 16 16\"><path d=\"M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z\"></path></svg></a></h1>"
        "<div class=\"nav-bar\"><div class=\"path\"><span>";
    
    sys_write(client, top, strlen(top));
    sys_write(client, display, strlen(display));
    
    const char* mid = 
        "</span></div><div class=\"controls\">"
        "<a href=\"javascript:history.back()\" class=\"btn btn-nav\">&lt;</a>"
        "<a href=\"javascript:history.forward()\" class=\"btn btn-nav\">&gt;</a>"
        "</div></div><ul>";
    sys_write(client, mid, strlen(mid));

    int dfd = sys_open(path, 0);
    if (dfd >= 0) {
        char buf[2048];
        int n;
        while ((n = sys_getdents64(dfd, (struct linux_dirent64*)buf, sizeof(buf))) > 0) {
            for (int pos = 0; pos < n; ) {
                struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
                if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0) {
                    const char* icon = resolve_icon(d->d_name, d->d_type);
                    sys_write(client, "<li>", 4);
                    
                    if (d->d_type == DT_DIR) {
                        sys_write(client, "<a href=\"", 9);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "/\"><img src=\"/", 14);
                        sys_write(client, icon, strlen(icon));
                        sys_write(client, "\" class=\"ico\"> ", 15);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "</a>", 4);
                        
                        if (!g_nozip) {
                            sys_write(client, " <a href=\"", 10);
                            sys_write(client, d->d_name, strlen(d->d_name));
                            sys_write(client, "/?zip=1\" class=\"btn btn-dl\" download>Download</a>", 49);
                        }
                    } else {
                        sys_write(client, "<a href=\"", 9);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "\" download><img src=\"/", 22);
                        sys_write(client, icon, strlen(icon));
                        sys_write(client, "\" class=\"ico\"> ", 15);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "</a> <a href=\"", 14);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "\" class=\"btn btn-dl\" download>Download</a>", 42);
                    }
                    
                    sys_write(client, "</li>", 5);
                }
                pos += d->d_reclen;
            }
        }
        sys_close(dfd);
    }
    
    const char* foot = "</ul><div style='margin-top:60px;opacity:0.3;font-size:0.75em;line-height:1.6'>"
                       "<p>A minimal, dependency-free HTTP server built with raw Linux syscalls.</p>"
                       "<p style='display:flex;align-items:baseline;gap:8px'>RawServe - baked with syscalls &bull; dev: "
                       "<a href=\"https://lahirux.dev\" style=\"color:inherit;text-decoration:underline;display:inline\">lahirux.dev</a></p>"
                       "</div></body></html>";
    sys_write(client, foot, strlen(foot));
}

void serve(int client, struct sockaddr_in* addr) {
    char req[2048];
    ssize_t n = sys_read(client, req, sizeof(req) - 1);
    if (n <= 0) {
        sys_close(client);
        return;
    }
    req[n] = '\0';

    if (strncmp(req, "GET ", 4) != 0) {
        sys_close(client);
        return;
    }

    char* start = req + 4;
    char* end = strchr(start, ' ');
    if (!end) {
        sys_close(client);
        return;
    }

    char path[256];
    size_t len = end - start;
    if (len >= sizeof(path)) len = sizeof(path) - 1;
    strncpy(path, start, len);
    path[len] = '\0';

    // Simple query param check
    bool zip_req = false;
    char* q = strchr(path, '?');
    if (q) {
        if (!g_nozip && strcmp(q, "?zip=1") == 0) zip_req = true;
        *q = '\0';
    }

    int status = 200;
    const char* p = path;
    while (*p == '/') p++;
    
    if (strncmp(p, "assets/", 7) == 0) {
        const unsigned char* svg = nullptr;
        unsigned int svg_len = 0;
        if (strcmp(p, "assets/folder.svg") == 0) { svg = assets_folder_svg; svg_len = assets_folder_svg_len; }
        else if (strcmp(p, "assets/file.svg") == 0) { svg = assets_file_svg; svg_len = assets_file_svg_len; }
        else if (strcmp(p, "assets/photo.svg") == 0) { svg = assets_photo_svg; svg_len = assets_photo_svg_len; }
        else if (strcmp(p, "assets/video.svg") == 0) { svg = assets_video_svg; svg_len = assets_video_svg_len; }
        else if (strcmp(p, "assets/audio.svg") == 0) { svg = assets_audio_svg; svg_len = assets_audio_svg_len; }
        
        if (svg != nullptr) {
            const char* h_ok = "HTTP/1.1 200 OK\r\nContent-Type: image/svg+xml\r\nContent-Length: ";
            sys_write(client, h_ok, strlen(h_ok));
            char sbuf[32];
            itoa(svg_len, sbuf, 10);
            sys_write(client, sbuf, strlen(sbuf));
            sys_write(client, "\r\n\r\n", 4);
            sys_write(client, svg, svg_len);
            sys_close(client);
            return;
        }
    }

    char target[512];
    if (strncmp(p, "assets/", 7) == 0) {
        build_path(target, g_assets, p + 7);
    } else {
        build_path(target, g_root, p[0] == '\0' ? "." : p);
    }

    int fd = sys_open(target, O_RDONLY);
    if (fd < 0) {
        status = 404;
        const char* err = "HTTP/1.1 404\r\nContent-Length: 0\r\n\r\n";
        sys_write(client, err, strlen(err));
    } else {
        struct stat st;
        if (sys_fstat(fd, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (zip_req) {
                    sys_close(fd);
                    stream_zip(client, target, p[0] == '\0' ? "root" : p);
                    goto done;
                }
                // check for trailing slash redirect
                if (strlen(path) > 0 && path[strlen(path) - 1] != '/') {
                    status = 301;
                    const char* redir = "HTTP/1.1 301\r\nLocation: ";
                    sys_write(client, redir, strlen(redir));
                    sys_write(client, path, strlen(path));
                    sys_write(client, "/\r\n\r\n", 5);
                    sys_close(fd);
                    goto done;
                }
                sys_close(fd);
                render_dir(client, target, path[0] == '\0' ? "/" : path);
            } else {
                const char* h_ok = "HTTP/1.1 200 OK\r\n";
                sys_write(client, h_ok, strlen(h_ok));
                
                if (has_extension(target, ".svg")) {
                    const char* ct = "Content-Type: image/svg+xml\r\n";
                    sys_write(client, ct, strlen(ct));
                }
                
                sys_write(client, "Content-Length: ", 16);
                char sbuf[20];
                itoa((int)st.st_size, sbuf, 10);
                sys_write(client, sbuf, strlen(sbuf));
                sys_write(client, "\r\n\r\n", 4);

                sys_sendfile(client, fd, 0, st.st_size);
                sys_close(fd);
            }
        } else {
            status = 500;
            sys_close(fd);
            const char* err = "HTTP/1.1 500\r\n\r\n";
            sys_write(client, err, strlen(err));
        }
    }

done:
    // py-style logging
    if (!g_quiet && !has_extension(path, ".svg")) {
        print_ip(addr->sin_addr.s_addr);
        print(" - - \"GET ");
        print(path);
        if (zip_req) print("?zip=1");
        print(" HTTP/1.1\" ");
        print_int(status);
        print(" -\n");
    }
    sys_close(client);
}

void show_help(const char* prog) {
    print("Usage: ");
    print(prog);
    print(" [options]\n\n");
    print("Options:\n");
    print("  -p <port>  Port to listen on (default: 8080)\n");
    print("  -d <dir>   Root directory to serve (default: .)\n");
    print("  -i <addr>  IP address to bind to (default: 0.0.0.0)\n");
    print("  -q         Quiet mode (no access logs)\n");
    print("  -n         Disable directory ZIP downloads\n");
    print("  -v         Show version info\n");
    print("  -h         Show this help message\n");
}

int str_to_int(const char* s) {
    int res = 0;
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res;
}

extern "C" int main(int argc, char** argv) {
    // poor man's getopt
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            g_port = str_to_int(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            g_root = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            g_bind = parse_ip(argv[++i]);
        } else if (strcmp(argv[i], "-q") == 0) {
            g_quiet = true;
        } else if (strcmp(argv[i], "-n") == 0) {
            g_nozip = true;
        } else if (strcmp(argv[i], "-v") == 0) {
            print("RawServe v0.2 - baked with syscalls\n");
            return 0;
        } else if (strcmp(argv[i], "-h") == 0) {
            show_help(argv[0]);
            return 0;
        }
    }

    int sfd = sys_socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return 1;

    int opt = 1;
    sys_setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_port);
    addr.sin_addr.s_addr = g_bind;

    if (sys_bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        print("Error: bind failed\n");
        return 1;
    }
    
    if (sys_listen(sfd, 10) < 0) {
        print("Error: listen failed\n");
        return 1;
    }

    if (!g_quiet) {
        print("Server listening on ");
        print(g_root);
        print(" at ");
        print_ip(g_bind);
        print(":");
        print_int(g_port);
        print("...\n");
    }

    while (true) {
        struct sockaddr_in caddr;
        socklen_t clen = sizeof(caddr);
        int cfd = sys_accept(sfd, (struct sockaddr*)&caddr, &clen);
        if (cfd >= 0) {
            serve(cfd, &caddr);
        }
    }
    return 0;
}
