# MarkedPDF

MarkedPDF is a lightweight command-line tool written in C++ that converts Markdown text files into PDF documents.

> Completely hand-written by ZirconiumTian (\^~\^)

## Third-Party Components

MarkedPDF uses [PoDoFo](https://github.com/podofo/podofo) as its PDF manipulation component. PoDoFo is licensed under the [MPL 2.0](https://www.mozilla.org/en-US/MPL/2.0/).

## Building

The PoDoFo library is required to compile MarkedPDF. Install it with the following commands:

```sh
sudo pacman -S podofo   # Arch Linux
sudo apt install podofo # Debian/Ubuntu
```

Then, from the project root:

```sh
cmake -S . -B build
cmake --build build
```

After building, you can run `build/markedpdf` to use it. To install it system-wide, copy it into any directory in your PATH, for example `sudo cp build/markedpdf /usr/local/bin/markedpdf`.

## License

MarkedPDF is licensed under [GNU General Public License 3.0](/LICENSE).