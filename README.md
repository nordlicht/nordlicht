These tools are inspired by http://moviebarcode.tumblr.com/movie-index - go
have a look!

The goal is to build navigation methods that use these barcodes as seek
sliders. Currently these are two prototypes to generate the barcode:

- Prototype 1 is a shell script, it needs ffmpeg and graphicsmagick. It's slow,
  but more precise. Call `./video-barcode *videofile*` to generate
  `*videofile*.png`.
- Prototype 2 is a C program depending only on the ffmpeg libraries. It's
  faster, but currently only uses keyframes and thus is less precise. Build it
  with `make`, then call `./video-barcode *videofile* *width* *height*` to
  generate `barcode.ppm`. It sometimes seems off by a few seconds and can't
  handle videos with 2048p-resolution yet.
