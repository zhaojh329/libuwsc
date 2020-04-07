# libuwsc

[1]: https://img.shields.io/badge/license-MIT-brightgreen.svg?style=plastic
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=plastic
[4]: https://github.com/zhaojh329/libuwsc/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg?style=plastic
[6]: https://github.com/zhaojh329/libuwsc/issues/new
[7]: https://img.shields.io/badge/release-3.3.5-blue.svg?style=plastic
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
[cyaSSl(Wolfssl)]: https://github.com/wolfSSL/wolfssl
[lua-ev]: https://github.com/brimworks/lua-ev

一个轻量的针对嵌入式Linux的基于[libev]的WebSocket客户端C库。
提供Lua绑定。


# 我为什么要选择libev？
libev尝试追随UNIX工具箱哲学，一次只干一件事，每次都做到最好。

# 特性
* 轻量 - 35KB（使用glibc,stripped）
* 全异步 - 使用[libev]作为其事件后端
* 支持SSL - OpenSSL, mbedtls and CyaSSl(wolfssl)
* 代码结构清晰，通俗易懂，亦适合学习
* Lua绑定

# 依赖
* [libev]
* [openssl] - 如果你选择openssl作为你的SSL后端
* [mbedTls(polarssl)] - 如果你选择mbedtls作为你的SSL后端
* [cyassl(wolfssl)] - 如果你选择wolfssl作为你的SSL后端
* [lua-ev] - 如果你使用Lua

# 安装依赖软件

    sudo apt install libev-dev libssl-dev

# 编译和安装

	git clone --recursive https://github.com/zhaojh329/libuwsc.git
	cd libuwsc
	mkdir build && cd build
	cmake ..
	make && sudo make install

# 安装到OpenWRT

    opkg update
    opkg list | grep libuwsc
    opkg install libuwsc-nossl

如果安装失败，你可以[自己编译](/BUILDOPENWRT_ZH.md)。

# 贡献代码
如果你想帮助[libuwsc](https://github.com/zhaojh329/libuwsc)变得更好，请参考
[CONTRIBUTING_ZH.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING_ZH.md)。

# 技术交流
QQ群：153530783

