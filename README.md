# libuwsc([中文](/README_ZH.md))

[1]: https://img.shields.io/badge/license-LGPL2-brightgreen.svg?style=plastic
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=plastic
[4]: https://github.com/zhaojh329/libuwsc/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg?style=plastic
[6]: https://github.com/zhaojh329/libuwsc/issues/new
[7]: https://img.shields.io/badge/release-2.0.5-blue.svg?style=plastic
[8]: https://github.com/zhaojh329/libuwsc/releases

[![license][1]][2]
[![PRs Welcome][3]][4]
[![Issue Welcome][5]][6]
[![Release Version][7]][8]

[libubox]: https://git.openwrt.org/?p=project/libubox.git
[ustream-ssl]: https://git.openwrt.org/?p=project/ustream-ssl.git
[openssl]: https://github.com/openssl/openssl
[mbedtls]: https://github.com/ARMmbed/mbedtls
[CyaSSl(wolfssl)]: https://github.com/wolfSSL/wolfssl

A Lightweight and fully asynchronous WebSocket client C library based on libubox for Embedded Linux.

`Keep Watching for More Actions on This Space`

# Dependencies
* [libubox]
* [ustream-ssl] - If you need to support SSL
* [mbedtls] - If you choose mbedtls as your SSL backend
* [CyaSSl(wolfssl)] - If you choose wolfssl as your SSL backend
* [openssl] - If you choose openssl as your SSL backend

# Install dependent packages

    sudo apt install libjson-c-dev

    git clone https://git.openwrt.org/project/libubox.git
    cd libubox && cmake -DBUILD_LUA=OFF . && sudo make install

    git clone https://git.openwrt.org/project/ustream-ssl.git
    cd ustream-ssl && cmake . && sudo make install

# Configure
See which configuration are supported

	~/libuwsc/$ mkdir build && cd build
	~/libuwsc/build$ cmake .. -L
	~/libuwsc/build$ cmake .. -LH

# Build and install

	~/libuwsc/build$ make && sudo make install
	
# Install on OpenWrt

    opkg update
    opkg list | grep libuwsc
    opkg install libuwsc-nossl

If the install command fails, you can [compile it yourself](/BUILDOPENWRT.md).

# Contributing
If you would like to help making [libuwsc](https://github.com/zhaojh329/libuwsc) better,
see the [CONTRIBUTING.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING.md) file.

# QQ group: 153530783

# If the project is helpful to you, please do not hesitate to star. Thank you!
