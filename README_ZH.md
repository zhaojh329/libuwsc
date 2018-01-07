# libuwsc

![](https://img.shields.io/badge/license-GPLV3-brightgreen.svg?style=plastic "License")

[libubox]: https://git.openwrt.org/?p=project/libubox.git
[ustream-ssl]: https://git.openwrt.org/?p=project/ustream-ssl.git
[openssl]: https://github.com/openssl/openssl
[mbedtls]: https://github.com/ARMmbed/mbedtls
[CyaSSl(wolfssl)]: https://github.com/wolfSSL/wolfssl

一个针对嵌入式Linux的基于[libubox]的WebSocket客户端C库。

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
Add new feed into "feeds.conf.default":

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

# 贡献代码
如果你想帮助[libuwsc](https://github.com/zhaojh329/libuwsc)变得更好，请参考
[CONTRIBUTING_ZH.md](https://github.com/zhaojh329/libuwsc/blob/master/CONTRIBUTING_ZH.md)。

# 技术交流
QQ群：153530783
