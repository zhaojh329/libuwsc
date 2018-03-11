更新feeds

    ./scripts/feeds update -a
    ./scripts/feeds install -a

在menuconfig中选择libuwsc，然后重新编译固件。

    Libraries  --->
        Networking  --->
            <*> libuwsc-mbedtls.................................... libuwsc (mbedtls)
            < > libuwsc-nossl....................................... libuwsc (NO SSL)
            < > libuwsc-openssl.................................... libuwsc (openssl)
            < > libuwsc-wolfssl.................................... libuwsc (wolfssl)
