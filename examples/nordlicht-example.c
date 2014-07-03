// Compile with: `cc nordlicht-example.c -lnordlicht -lpthread -o nordlicht-example`

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <nordlicht.h>

void print_error() {
    // Print a description of the last error
    printf("Error: %s\n", nordlicht_error());
}

int main(int argc, const char **argv) {
    nordlicht *n;

    char *input_file = "video.mp4";
    int width = 1000;
    int height = 100;

    /*** USE CASE 1: Generate a barcode in the background ***/

    // Initialize a new nordlicht:
    n = nordlicht_init(input_file, width, height);
    if (n == NULL) {
        print_error();
        exit(1);
    }

    // Generate the barcode:
    if (nordlicht_generate(n) != 0) {
        print_error();
        exit(1);
    }

    // You can access the pixel buffer, it's in 32-bit BGRA format:
    const unsigned char *data = nordlicht_buffer(n);

    // You can also write the barcode to a PNG file:
    if (nordlicht_write(n, "barcode.png") != 0) {
        print_error();
        exit(1);
    }

    // When you're done, free the nordlicht:
    nordlicht_free(n);


    /*** USE CASE 2: Generate a barcode "live", into a buffer supplied by you ***/

    // Initialize a new nordlicht:
    n = nordlicht_init(input_file, width, height);
    if (n == NULL) {
        print_error();
        exit(1);
    }

    // Allocate a buffer of the correct size and tell nordlicht to use it:
    unsigned char *data2 = malloc(nordlicht_buffer_size(n));
    nordlicht_set_buffer(n, data2);

    // Set the style to "horizontal", which will compress the frames to a column:
    nordlicht_set_style(n, NORDLICHT_STYLE_HORIZONTAL);

    // Set the strategy to "live", which gives you a rough approximation
    // faster. This is nice if you generate the barcode in front of the user's eyes:
    nordlicht_set_strategy(n, NORDLICHT_STRATEGY_LIVE);

    // Start the generation in a thread:
    pthread_t thread;
    pthread_create(&thread, NULL, (void*(*)(void*))nordlicht_generate, n);

    // Until it's done, check the progress ten times a second:
    float progress = 0;
    while (progress < 1) {
        progress = nordlicht_progress(n);
        // (Display the progress)
        // (Redraw the buffer)
        usleep(100000);
    }
    pthread_join(thread, NULL);

    // Since you called nordlicht_set_buffer, you own the buffer:
    free(data2);

    // Free the nordlicht:
    nordlicht_free(n);
}
