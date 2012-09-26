JSMESS
======
JSMESS is an attempt to port [MESS](http://mess.org/) /
[MAME](http://mamedev.org/) to JavaScript using
[Emscripten](https://github.com/kripken/emscripten).

Why?
----
[Jason Scott](http://jsmess.textfiles.com/) says it best:
> The MESS program can emulate (or begin to emulate) a majority of home
> computers, and continues to be improved frequently. By porting this program
> into the standardized and cross-platform Javascript language, it will be
> possible to turn computer history and experience into the same embeddable
> object as movies, documents, and audio enjoy.

Building
--------

### Prerequisites ###
* [LLVM and Clang 3.1](http://llvm.org/releases/download.html#3.1)

  Emscripten uses these to work its magic. As this is the latest version of LLVM
  and Clang, chances are you will need to compile it from scratch.

  Too lazy to read the LLVM build instructions? Make sure you have ```g++```
  installed (not just ```gcc```!) and run these commands:

  ```
  wget http://llvm.org/releases/3.1/llvm-3.1.src.tar.gz
  wget http://llvm.org/releases/3.1/clang-3.1.src.tar.gz
  tar xf llvm-3.1.src.tar.gz
  cd llvm-3.1.src/tools
  tar xf ../../clang-3.1.src.tar.gz
  mv clang-3.1.src clang
  cd ..
  ./configure
  make
  sudo make install
  ```

* [Node.js](http://nodejs.org)

  You can usually install this from your platform's package repository, e.g.:

  ```
  sudo apt-get install nodejs
  ```

* ColecoVision BIOS and Cosmo Fighter 2

  The ColecoVision BIOS should be in the ```bin``` directory with the filename
  ```coleco.zip```, and Cosmo Fighter 2 should be in the ```bin``` directory
  with the filename ```cosmofighter2.zip```. For legal reasons, we do not
  distribute these with JSMESS.

  [You can probably find them on your own, though.](http://lmgtfy.com/?q=ColecoVision+BIOS)

  To try out other games, try changing the hardcoded data in ```makefile```.

* Set Up Emscripten

  First, because ```git``` is stupid, run this command to grab the emscripten
  sources:

  ```git submodule update --init --recursive```

  Then, test out ```emcc```:

  ```
  cd third_party/emscripten
  ./emcc
  ```

  Edit ```~/.emscripten``` to point to your LLVM installation and Emscripten
  directory.

### Building ###

1. Run ```make```. Go out for a walk; it'll take a bit.
2. Open up ```messtiny.html``` in a recent version of Chrome or Firefox.

Relevant Links
--------------
* [The blog post that started it all](http://ascii.textfiles.com/archives/3375)
* [Wiki page](http://www.archiveteam.org/index.php?title=Javascript_Mess)



