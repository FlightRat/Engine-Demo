# Lua 预编译依赖

Lua 5.3.5，头文件共用，lua53.lib 分别位于 Debug/lib 和 Release/lib。Debug 保留原有 /MT 二进制，因此链接 /MDd 工程仍可能出现 LNK4098。Release 使用 /MD，由此前校验过 SHA-256 的官方源码归档生成。源码：https://www.lua.org/ 。许可见 LICENSE.txt。预编译文件随仓库提供，普通构建无需重建依赖。
