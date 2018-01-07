# libuwsc([中文](https://github.com/zhaojh329/libuwsc/blob/master/README_ZH.md))

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

[libubox]: https://git.openwrt.org/?p=project/libubox.git
[ustream-ssl]: https://git.openwrt.org/?p=project/ustream-ssl.git
[openssl]: https://github.com/openssl/openssl
[mbedtls]: https://github.com/ARMmbed/mbedtls
[CyaSSl(wolfssl)]: https://github.com/wolfSSL/wolfssl

A WebSocket client C library based on libubox for Embedded Linux.

`Keep Watching for More Actions on This Space`

# Dependencies
* [libubox]
* [ustream-ssl] - If you need to support SSL
* [mbedtls] - If you choose mbedtls as your SSL backend
* [CyaSSl(wolfssl)] - If you choose wolfssl as your SSL backend
* [openssl] - If you choose openssl as your SSL backend

# Configure
See which configuration are supported
	~/libuwsc/$ mkdir build && cd build
	~/libuwsc/build$ cmake .. -L
	~/libuwsc/build$ cmake .. -LH

# Build and install

	~/libuwsc/build$ make && sudo make install
	
# How to use on OpenWRT
add new feed into "feeds.conf.default":

    src-git libuwsc https://github.com/zhaojh329/libuwsc-feed.git

Install libuwsc packages:

    ./scripts/feeds update libuwsc
    ./scripts/feeds install -a -p libuwsc

Select package libuwsc in menuconfig and compile new image.

    Libraries  --->
        Networking  --->
            <*> libuwsc-mbedtls.................................... libuwsc (mbedtls)
            < > libuwsc-nossl....................................... libuwsc (NO SSL)
            < > libuwsc-openssl.................................... libuwsc (openssl)
            < > libuwsc-wolfssl.................................... libuwsc (wolfssl)

# Contributing
If you would like to help making [libuwsc](https://github.com/zhaojh329/libuwsc) better,
see the [CONTRIBUTING.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING.md) file.
