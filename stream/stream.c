
#include <guilib.h>
#include "netsock.h"

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

static int g_server;

static void sig_handler(int sig)
{
        if (sig == SIGPIPE) {
                fprintf(stderr, "server: got SIGPIPE\n");
                abort();
        } else {
                abort();
        }
}

static void *key_reader_thread(void *winptr)
{
        struct gui_window *window = winptr;
        char event;

        while (window->__key_reader_run) {
                event = soc_recv_char(window->__client);

                switch (event) {
                case 'K':
                case 'k': {
                        bool pressed = (event == 'K');
                        int key = soc_recv_number(window->__client);
                        printf("server: key %d %s\n", key, pressed ? "pressed" : "released");
                        if (window->__callback != NULL) {
                                window->__callback(window, key, pressed);
                        }
                        break;
                }
                case 'M':
                          window->__mouse_x = soc_recv_number(window->__client);
                          window->__mouse_y = soc_recv_number(window->__client);
                          //printf("server: mouse at %d:%d\n", window->__mouse_x, window->__mouse_y);
                          window->__waiting_for_mouse = false;
                          break;
                default:
                          printf("server: unkwonwn event\n");
                }
                window->__waiting_for_interrupt = false;
        }
        return NULL;
}

void gui_bootstrap(void)
{
        if (signal(SIGPIPE, sig_handler)) {
                fprintf(stderr, "gui_bootstrap: setting signal handler fail\n");
                abort();
        }
        g_server = soc_create_server(7777);
}

void gui_finalize(void)
{
        soc_close(g_server);
}

void gui_create(struct gui_window *window, unsigned int width, unsigned int height)
{
        window->__width = width;
        window->__height = height;
        window->__length = (unsigned long)width * height;

        window->__callback = NULL;
        window->__waiting_for_mouse = false;
        window->__waiting_for_interrupt = false;
        window->__mouse_x = 0;
        window->__mouse_y = 0;
        window->__key_reader_run = true;

        printf("server: waiting for client\n");
        window->__client = soc_server_accept(g_server);

        window->__raw_pixels = malloc(window->__length * sizeof(unsigned));
        if (window->__raw_pixels == NULL) {
                fprintf(stderr, "no memory\n");
                abort();
        }

        pthread_create(&window->__key_thread, NULL, key_reader_thread, window);

        soc_send_char(window->__client, 'R');
        soc_send_number(window->__client, width);
        soc_send_number(window->__client, height);
}

void gui_destroy(struct gui_window *window)
{
        window->__key_reader_run = false;
        pthread_join(window->__key_thread, NULL);
}

unsigned int gui_width(const struct gui_window *window)
{
        return window->__width;
}

unsigned int gui_height(const struct gui_window *window)
{
        return window->__height;
}

void gui_set_pixel(struct gui_window *window, unsigned x, unsigned y, unsigned color)
{
#ifdef DEBUG
        if (x >= window->__width || y >= window->__height) {
                fprintf(stderr, "gui_set_pixel(): out of bounds\n");
        }
#endif
        gui_set_pixel_raw(window, (unsigned long)y * window->__width + x, color);
}

void gui_set_pixel_raw(struct gui_window *window, unsigned long i, unsigned color)
{
        window->__raw_pixels[i] = color;
}

int gui_set_pixel_safe(struct gui_window *window, unsigned x, unsigned y, unsigned color)
{
        if (x < window->__width && y < window->__height) {
                gui_set_pixel(window, x, y, color);
                return 0;
        } else {
                return EINVAL;
        }
}

unsigned *gui_raw_pixels(const struct gui_window *window)
{
        return window->__raw_pixels;
}

void gui_draw(const struct gui_window *window)
{
        soc_send_char(window->__client, 'b');
        soc_send_number(window->__client, window->__length * sizeof(int));
        soc_send(window->__client, window->__raw_pixels, window->__length * sizeof(int));
}

void gui_key_hook(struct gui_window *window, key_hook_t callback)
{
        window->__callback = callback;
}

void gui_mouse(const struct gui_window *window, int *x, int *y)
{
        ((struct gui_window *)window)->__waiting_for_mouse = true;
        soc_send_char(window->__client, 'm');
        while (window->__waiting_for_mouse) {
                usleep(10000);
        }

        *x = window->__mouse_x;
        *y = window->__mouse_y;
}

void gui_wfi(struct gui_window *window)
{
        window->__waiting_for_interrupt = true;
        while (window->__waiting_for_interrupt) {
                usleep(10000);
        }
}
