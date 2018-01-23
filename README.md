# libuwsc([中文](/README_ZH.md))

[1]: https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=plastic
[4]: https://github.com/zhaojh329/libuwsc/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg?style=plastic
[6]: https://github.com/zhaojh329/libuwsc/issues/new
[7]: https://img.shields.io/badge/release-1.1.0-blue.svg?style=plastic
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

Lightweight WebSocket client C library based on libubox for Embedded Linux.

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
Update feeds:

    ./scripts/feeds update -a
    ./scripts/feeds install -a

For chaos_calmer(15.05), you need to modify the Makefile: feeds/packages/libs/libuwsc/Makefile

    PKG_SOURCE_URL=https://github.com/zhaojh329/libuwsc.git
    # Add the following two lines
    PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
    PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
    # And comment the line below
    #PKG_MIRROR_HASH:=4aada7e2941fb9f099869c9dc10ef6411f1c355c3b2f570011b91e42feffbfdd

Select libuwsc in menuconfig and compile new image.

    Libraries  --->
        Networking  --->
            <*> libuwsc-mbedtls.................................... libuwsc (mbedtls)
            < > libuwsc-nossl....................................... libuwsc (NO SSL)
            < > libuwsc-openssl.................................... libuwsc (openssl)
            < > libuwsc-wolfssl.................................... libuwsc (wolfssl)

# Contributing
If you would like to help making [libuwsc](https://github.com/zhaojh329/libuwsc) better,
see the [CONTRIBUTING.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING.md) file.
