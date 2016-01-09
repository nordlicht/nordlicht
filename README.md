# nordlicht

**nordlicht** is a C library that creates moodbars from video files. For a general introduction, installation and usage instructions, please visit <http://nordlicht.github.io/>.

## Building from source

Get CMake, FFmpeg/libav, [popt](http://freecode.com/projects/popt), and [help2man](https://www.gnu.org/software/help2man/), and issue the following commands to create the `nordlicht` binary:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

To run the test suite, make sure some `video.mp4` exists and run the `testsuite` binary.

## Contributing

Development of *nordlicht* happens on GitHub. Please report any bugs or ideas to the [issue tracker](https://github.com/nordlicht/nordlicht/issues). To contribute code, fork the repository and submit a pull request.

You can also help by packaging the software for your favorite operating system, or writing an integration for your favorite video player. Even rough prototypes are highly appreciated!

## License: GPLv2+

See [LICENSE.md] for details.
