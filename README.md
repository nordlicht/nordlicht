# Video Barcodes FTW!

These tools are inspired by http://moviebarcode.tumblr.com/movie-index - go
have a look!

The goal is to build navigation methods that use video barcodes as seek
sliders. Currently these are two prototypes to generate the barcode:

- Prototype 1 is a shell script, it needs ffmpeg and graphicsmagick. It's slow,
  but more precise. Call `./video-barcode videofile` to generate
  `videofile.png`. For more control, edit the script :P
- Prototype 2 is a C program depending only on the ffmpeg libraries. It's
  faster, but currently only uses keyframes and thus is less precise. Build it
  with `make`, then call `./video-barcode videofile width height` to generate
  `barcode.ppm`. You can then use your favourite graphics program to convert it
  to a real image format.

  Known issues: The barcode seems off by a few seconds and something
  goes wrong with 2048p videos yet.
