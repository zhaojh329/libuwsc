# libuwsc([中文](/README_ZH.md))

[1]: https://img.shields.io/badge/license-MIT-brightgreen.svg?style=plastic
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=plastic
[4]: https://github.com/zhaojh329/libuwsc/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg?style=plastic
[6]: https://github.com/zhaojh329/libuwsc/issues/new
[7]: https://img.shields.io/badge/release-3.3.2-blue.svg?style=plastic
[8]: https://github.com/zhaojh329/libuwsc/releases
[9]: https://travis-ci.org/zhaojh329/libuwsc.svg?branch=master
[10]: https://travis-ci.org/zhaojh329/libuwsc

[![license][1]][2]
[![PRs Welcome][3]][4]
[![Issue Welcome][5]][6]
[![Release Version][7]][8]
[![Build Status][9]][10]

[libev]: http://software.schmorp.de/pkg/libev.html
[openssl]: https://github.com/openssl/openssl
[mbedtls(polarssl)]: https://github.com/ARMmbed/mbedtls
[CyaSSl(wolfssl)]: https://github.com/wolfSSL/wolfssl
[lua-ev]: https://github.com/brimworks/lua-ev

A Lightweight and fully asynchronous WebSocket client library based on [libev] for Embedded Linux.
And provide Lua-binding.


# Why should I choose libev?
 libev tries to follow the UNIX toolbox philosophy of doing one thing only, as good as possible.

# Features
* Lightweight - 35KB（Using glibc,stripped）
* Fully asynchronous - Use [libev] as its event backend
* Support ssl - OpenSSL, mbedtls and CyaSSl(wolfssl)
* Code structure is concise and understandable, also suitable for learning
* Lua-binding

# Dependencies
* [libev]
* [openssl] - If you choose openssl as your SSL backend
* [mbedtls(polarssl)] - If you choose mbedtls as your SSL backend
* [cyassl(wolfssl)] - If you choose wolfssl as your SSL backend
* [lua-ev] - If you use Lua

# Install dependent packages

    sudo apt install libev-dev libssl-dev

# Build and install

	git clone --recursive https://github.com/zhaojh329/libuwsc.git
	cd libuwsc
	mkdir build && cd build
	cmake ..
	make && sudo make install
	
# Install on OpenWrt

    opkg update
    opkg list | grep libuwsc
    opkg install libuwsc-nossl

If the install command fails, you can [compile it yourself](/BUILDOPENWRT.md).

# Contributing
If you would like to help making [libuwsc](https://github.com/zhaojh329/libuwsc) better,
see the [CONTRIBUTING.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING.md) file.

