# Yet Another Raft Implementation

## Build Requirements

* A c++ compiler with c++14 support (clang or gcc).
* CMake at least in version 3.0.

## Build

Starting from the projects root directoy:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

## Usage

Currently, only the unit test application can be executed, which makes not
so much fun like it sounds. After running `make`, you can find and execute
the `raft-test` binary within the `build` directory.

## References

* [Raft protocol and demonstration](https://raft.github.io/)
