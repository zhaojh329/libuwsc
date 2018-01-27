# libuwsc

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

一个轻量的针对嵌入式Linux的基于[libubox]的WebSocket客户端C库。

`请保持关注以获取最新的项目动态`

# 依赖
* [libubox]
* [ustream-ssl] - 如果你需要支持SSL
* [mbedtls] - 如果你选择mbedtls作为你的SSL后端
* [CyaSSl(wolfssl)] - 如果你选择wolfssl作为你的SSL后端
* [openssl] - 如果你选择openssl作为你的SSL后端

# 配置
查看支持哪些配置选项

	~/libuwsc/$ mkdir build && cd build
	~/libuwsc/build$ cmake .. -L
	~/libuwsc/build$ cmake .. -LH

# 编译和安装

	~/libuwsc/build$ make && sudo make install

# 如何在OpenWRT中使用
更新feeds:

    ./scripts/feeds update -a
    ./scripts/feeds install -a

对于chaos_calmer(15.05), 你需要修改Makefile: feeds/packages/libs/libuwsc/Makefile

    PKG_SOURCE_URL=https://github.com/zhaojh329/libuwsc.git
    # Add the following two lines
    PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
    PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
    # And comment the line below
    #PKG_MIRROR_HASH:=4aada7e2941fb9f099869c9dc10ef6411f1c355c3b2f570011b91e42feffbfdd

在menuconfig中选择libuwsc，然后重新编译固件。

    Libraries  --->
        Networking  --->
            <*> libuwsc-mbedtls.................................... libuwsc (mbedtls)
            < > libuwsc-nossl....................................... libuwsc (NO SSL)
            < > libuwsc-openssl.................................... libuwsc (openssl)
            < > libuwsc-wolfssl.................................... libuwsc (wolfssl)

# 贡献代码
如果你想帮助[libuwsc](https://github.com/zhaojh329/libuwsc)变得更好，请参考
[CONTRIBUTING_ZH.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING_ZH.md)。

# 技术交流
QQ群：153530783
