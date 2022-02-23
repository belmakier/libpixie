# libpixie {#mainpage}

@tableofcontents

libpixie is a C++ library for reading the XIA Pixie16 data format (see
https://xia.com/wp-content/uploads/2018/04/Pixie16_UserManual.pdf)
into C++ classes. libpixie also handles "event building" - that is,
grouping several sub-events (referred to as "measurements" here)
together using some coincidence window. 

## Installation

Installation is very simple, you'll just need to get the source code
and compile to get going. First clone the repository using

```
git clone https://github.com/belmakier/libpixie.git
```

Then change directory and make some build directories:

```
cd libpixie;
mkdir lib;
mkdir obj;
```

Change directory again and compile

```
cd src
make
```

The install directories are specified in the Makefile, by default they
are ```$HOME/.local/include``` for headers and ```$HOME/.local/lib```
for dynamic libraries. If these don't exist you'll need to create
them. After that, you can install with

```
make install
```

Make sure the ```$HOME/.local/lib``` directory is in your
```$LD_LIBRARY_PATH```, and you're good to go!

# Developers {#authors}
+ Timothy Gray <graytj@ornl.gov>
