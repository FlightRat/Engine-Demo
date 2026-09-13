# Assimp 预编译依赖

头文件共用，Debug/lib、Debug/bin 保留原有 assimp-vc143-mtd.lib/.dll。Release/lib、Release/bin 的 assimp-vc143-mt.lib/.dll 按现有头文件标记的提交 553fbc1fdb9bf1d3e1b7a2382e4727bd71a4aee3 编译，使用动态 Release CRT。源码：https://github.com/assimp/assimp 。许可见 LICENSE.txt。预编译文件随仓库提供，普通构建无需重建依赖。
