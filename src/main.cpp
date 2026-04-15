#include "syscalls.h"
#include "string_utils.h"

// Hardcoded for now, maybe move to args later?
#define PORT 8080

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
                if (strcmp(d->d_name, ".") != 0) {
                    const char* icon = resolve_icon(d->d_name, d->d_type);
                    sys_write(client, "<li>", 4);
                    
                    if (d->d_type == DT_DIR) {
                        sys_write(client, "<a href=\"", 9);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "/\"><img src=\"/", 14);
                        sys_write(client, icon, strlen(icon));
                        sys_write(client, "\" class=\"ico\"> ", 15);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "</a> <a href=\"", 14);
                        sys_write(client, d->d_name, strlen(d->d_name));
                        sys_write(client, "/\" class=\"btn btn-dl\" download>Download</a>", 43);
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

    int status = 200;
    const char* p = path;
    while (*p == '/') p++;
    const char* target = (p[0] == '\0') ? "." : p;

    int fd = sys_open(target, O_RDONLY);
    if (fd < 0) {
        status = 404;
        const char* err = "HTTP/1.1 404\r\nContent-Length: 0\r\n\r\n";
        sys_write(client, err, strlen(err));
    } else {
        struct stat st;
        if (sys_fstat(fd, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // check for trailing slash redirect
                if (len > 0 && path[len - 1] != '/') {
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

                char fbuf[4096];
                ssize_t rn;
                while ((rn = sys_read(fd, fbuf, sizeof(fbuf))) > 0) {
                    sys_write(client, fbuf, rn);
                }
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
    if (!has_extension(path, ".svg")) {
        print_ip(addr->sin_addr.s_addr);
        print(" - - \"GET ");
        print(path);
        print(" HTTP/1.1\" ");
        print_int(status);
        print(" -\n");
    }
    sys_close(client);
}

extern "C" int main(int argc, char** argv) {
    int sfd = sys_socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) return 1;

    int opt = 1;
    sys_setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (sys_bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        print("Error: bind failed\n");
        return 1;
    }
    
    if (sys_listen(sfd, 10) < 0) {
        print("Error: listen failed\n");
        return 1;
    }

    print("Server listening on port ");
    print_int(PORT);
    print("...\n");

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
