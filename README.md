# BtEmbedded

A static library for embedded devices to operate on Bluetooth devices (classic, not LE).

## Features

### Supported target platforms

BtEmbedded can be built on the following platforms:

- Nintendo Wii
- Linux
- PLANNED: Nintendo Wii's Starlet processor (cIOS)

## Architecture and design goals

BtEmbedded tries to be portable and as fast and small as possible. The
structure of the code is as follows:

```
bt-embedded/
    backends/   # Platform backends
        wii.c   # Platform backend for the Nintendo Wii PPC processor
    backend.h   # Interface for platform backends
    
    drivers/    # Drivers for the Bluetooth controller
        wii.c   # Driver for the Wii's Bt controller
    drivers.h   # Interface for the Bluetooth drivers

    # BtEmbedded source code, common for all targets:
    hci.c
    hci.h
    ...
```

BtEmbedded is typically built statically, which means that the platform backend
and the Bluetooth driver are selected at build time.

The next two sections explain what code is expected for platform backends and
device drivers; the following sections describe other design decisions.

### Platform backends

A platform backend's main function is that of providing a function which can be
used to send data to the Bluetooth HCI controller, and delivering back HCI
events to BtEmbedded. The API operates on raw data buffers: the platform
backend does not need to perform any parsing of the data.

Beside the platform backend code, there's also a `platform_defs.h` file, which
can be used to specify the alignment requirement for the memory buffers
exchanged with the HCI controller, and also provide a different implementation
of malloc/free, if needed.

### Bluetooth drivers

A Bluetooth driver must bring up the HCI controller to a working state. This
can be as simple as invoking the HCI Reset command, or can require uploading
firmware or perform some vendor-specific commands. Drivers operate on the HCI
controller using the very same API that BtEmbedded provides to external
clients. However, they have also access to BtEmbedded's internal data
structures, so they can alter some private parameters.

The reason why the Bluetooth driver is not integrated as part of the platform
backend is that while on most embedded platform there's a one to one relation
between the platform and the HCI controller, there can be platforms which
support different types of controllers; viceversa, it's also possible that the
same controller is found on different platforms (in the case of the Nintendo
Wii, we do have two different platforms, one for the PPC processor and the
other one for the ARM one, and they both work with the same bluetooth
controller).

### Optimizing data transfers

BtEmbedded uses the BteBuffer structure (defined in
[buffer.h](bt-embedded/buffer.h)) for all data transfers between the HCI
controller and the client. This structure is a reference counted linked list of
buffers: the reference count allows avoiding making copies of the data, because
the buffers will be freed only when the last reference is dropped.

The `BteBufferReader` and `BteBufferWriter` APIs provide a convenient way to
read and write data from the BteBuffer lists without having to worry about
fragmentation and header fields (which are automatically processed), and offer
direct access to the memory buffer, therefore avoiding unnecessary copies.

The structure definition is public, therefore clients are free to allocate
buffer as they see fit (statically or dynamically), and its `free_func()`
method allows specifying a `free()`-like function that will be invoked at the
end of the buffer's lifetime.

### Optimizing for code size

The project is built with the GCC's `-ffunction-sections` option, which allows
the compiler to pick individual functions when linking the library into a
program; and BtEmbedded itself is written in such a way that the code for
parsing HCI events is passed to the core only in form of a callback, without a
direct dependency. In practice, this means that if neither the client nor the
driver call the `bte_hci_read_local_name()` function (for example), the code to
parse the HCI "Read Local Name" command result will not be included into the
program's binary.

### Support for multiple concurrent clients

There might be cases where different parts of a program need to interact with
the Bluetooth controller for different needs, without knowing of each other;
for example, in the Nintendo Wii the libogc library communicates with the
Bluetooth controller to read the Wiimote's data, while other code in the
application might need Bluetooth for talking to a keyboard. In such cases it's
important that these different parts of the code don't step onto each other
feet, which would typically happen if one part of the code registers a callback
for some HCI event that is also interesting for the other part. BtEmbedded is
designed to support these use cases: different clients can issue different
commands at the same time, and the result will be delivered to the correct
client. In cases where properly delivering concurrent messages is impossible,
BtEmbedded will return an error to the second client, simulating a situation
where the HCI controller is busy (clients need to be able to handle this
situation anyway).

### Testability

BtEmbedded comes with a test suite that covers all of its functionality. It's
also executed under [valgrind](https://valgrind.org/), to detect memory leaks
and errors.


## Building and running

### Nintendo Wii

##### 1) Install `devkitPPC`

- Download and install [devkitPPC](https://devkitpro.org/wiki/Getting_Started)
- Make sure to install the `devkitppc-cmake` package when using `pacman`

##### 2) (optional) Build `OpenOBEX`

If you want to build the `obex.c` example to test file transfers, you need to
build the OpenOBEX library. This is easily done by using the version at [this
branch](https://gitlab.com/mardy/mainline/-/tree/wii), which can be compiled as
follows:

    mkdir build && cd build
    cmake -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Wii.cmake" \
          -DCMAKE_BUILD_TYPE=Debug -DOPENOBEX_INET=OFF -DOPENOBEX_FD=OFF ..
    make && sudo -E make install

##### 3) Build `libbt-embedded.a`

1. `mkdir build && cd build`
2. Configure it with CMake:
  &ensp; `cmake -DCMAKE_TOOLCHAIN_FILE="$DEVKITPRO/cmake/Wii.cmake" -DBUILD_EXAMPLE=ON ..`
3. `make` (or `ninja` if configured with `-G Ninja`)
4. `libbt-embedded.a` will be generated.

##### 4) Run the examples.

The compiled examples will be in the `build/examples` folder. You can run them
on the Wii console, or in the Dolphin emulator (but you need to setup
[bluetooth passthrough](https://wiki.dolphin-emu.org/index.php?title=Bluetooth_Passthrough)
first).


### Linux

##### 1) Dependencies

To build bt-embedded, you'll need `cmake`, `libusb` and a C compiler (plus a
C++ one if you want to build the tests). In a Debian based distribution, these
can be installed like this:

    sudo apt install cmake libusb-1.0-0-dev

##### 2) (optional) Build `OpenOBEX`

If you want to build the `obex.c` example to test file transfers, you need to
build the OpenOBEX library. This is easily done by using the version at [this
branch](https://gitlab.com/mardy/mainline/-/tree/wii), which can be compiled as
follows:

    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Debug ..
    make && sudo -E make install

##### 3) Build `libbt-embedded.a`

1. `mkdir build && cd build`
2. Configure it with CMake:
  &ensp; `cmake -DBUILD_EXAMPLE=ON ..`
3. `make` (or `ninja` if configured with `-G Ninja`)
4. `libbt-embedded.a` will be generated

##### 4) Run the examples.

The compiled examples will be in the `build/examples` folder. In order to run
them, you'll first need to setup the permissions on your bluetooth adaptor as
explained [here](bt-embedded/backends/README-libusb.md).

## Credits

- [libogc's lwBT](https://github.com/devkitPro/libogc/tree/master/lwbt), which
  has been used as a base for this work.
