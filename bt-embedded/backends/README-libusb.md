# libusb backend for bt-embedded

This backend allows using bt-embedded on any operating system where
[libusb](https://libusb.info/) is supported. According to libusb's website,
these are Linux, macOS, Windows (Vista and newer), Android, OpenBSD/NetBSD,
Haiku, Solaris.  However, bt-embedded has been tested only on Linux, so the
information below is probably only relevant to Linux users. If you happen to
use a different OS supported by libusb and encounter some issues running
bt-embedded on it (or even better, if you manage to run it *without* issues!)
please let us know by filing an issue on bt-embedded, so that we can help with
the deployment and eventually add the missing information here.

Generic information on running libusb applications on different OSes can be
find [in the libusb
FAQ](https://github.com/libusb/libusb/wiki/FAQ#user-content-Running_libusb).


# Linux: setting up udev rules

In order to be able to run bt-embedded applications without requiring root
privileges, a `udev` rule needs to be setup:

1. Find the Vendor ID and Product ID of your bluetooth adapter (using `lsusb`)
2. Paste the following into /etc/udev/rules.d/52-bt-embedded.rules
   (/lib/udev/rules.d/52-bt-embedded.rules if making a package). Replace 'VID'
   and 'PID' with the Vendor ID and Product ID respectively:

    SUBSYSTEM=="usb", ATTRS{idVendor}=="VID", ATTRS{idProduct}=="PID", TAG+="uaccess"

3. Reload udev rules: `sudo udevadm control --reload-rules`
4. Reinsert the adapter.
5. Switch off Bluetooth in the System Settings, otherwise the device will be
   held busy by Bluez.
