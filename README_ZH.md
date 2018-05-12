# libuwsc

[1]: https://img.shields.io/badge/license-LGPL2-brightgreen.svg?style=plastic
[2]: /LICENSE
[3]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=plastic
[4]: https://github.com/zhaojh329/libuwsc/pulls
[5]: https://img.shields.io/badge/Issues-welcome-brightgreen.svg?style=plastic
[6]: https://github.com/zhaojh329/libuwsc/issues/new
[7]: https://img.shields.io/badge/release-2.0.1-blue.svg?style=plastic
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

一个轻量的针对嵌入式Linux的基于[libubox]的WebSocket客户端C库。

`请保持关注以获取最新的项目动态`

# 依赖
* [libubox]
* [ustream-ssl] - 如果你需要支持SSL
* [mbedtls] - 如果你选择mbedtls作为你的SSL后端
* [CyaSSl(wolfssl)] - 如果你选择wolfssl作为你的SSL后端
* [openssl] - 如果你选择openssl作为你的SSL后端

# 安装依赖软件

    sudo apt install libjson-c-dev

    git clone https://git.openwrt.org/project/libubox.git
    cd libubox && cmake -DBUILD_LUA=OFF . && sudo make install

    git clone https://git.openwrt.org/project/ustream-ssl.git
    cd ustream-ssl && cmake . && sudo make install

# 配置
查看支持哪些配置选项

	~/libuwsc/$ mkdir build && cd build
	~/libuwsc/build$ cmake .. -L
	~/libuwsc/build$ cmake .. -LH

# 编译和安装

	~/libuwsc/build$ make && sudo make install

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

# 如果该项目对您有帮助，请随手star，谢谢！
