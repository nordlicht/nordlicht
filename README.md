# nordlicht

**nordlicht** is a C library that creates colorful timebars from video and audio files. For a general introduction, installation and usage instructions, please visit <http://nordlicht.github.io/>.

## Building from source

Get CMake, FFmpeg/libav, [popt](http://freecode.com/projects/popt), and [help2man](https://www.gnu.org/software/help2man/), and issue the following commands to create the `nordlicht` binary:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

To run the test suite, run `make check`. Note that the test suite will download a ~20 MB test video from Wikimedia Commons.

## Contributing

Development of *nordlicht* happens on GitHub. Please report any bugs or ideas to the [issue tracker](https://github.com/nordlicht/nordlicht/issues). To contribute code, fork the repository and submit a pull request.

You can also help by packaging the software for your favorite operating system, or writing an integration for your favorite video player. Even rough prototypes are highly appreciated!

Please note that this project is released with a [Contributor Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project you agree to abide by its terms.

## License: GPLv2+

See [LICENSE.md](LICENSE.md) for details.
