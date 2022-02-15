# efiboot

a efiboot tool on windows


# 笔记

这个项目到目前就这样吧。

该项目还不完善，还有很多东西需要改善。

比如：完成的参数检查。

使用办法:

//查看boot顺序以及所有boot项

efiboot

或

efiboot --bootorder

//查看某个boot项的具体信息

//查看boot0002项

efiboot --bootoption boot0002


//添加一个boot项

efiboot --add winpe \\\\.\P: \EFI\Boot\bootx64.efi
