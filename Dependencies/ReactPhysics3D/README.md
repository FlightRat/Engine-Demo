# ReactPhysics3D 预编译依赖

版本 0.10.2，官方 tag 对应提交 cd958bbc0c6e84a869388cba6613f10cc645b3cb，使用单精度。头文件共用；reactphysics3d.lib 分别位于 Debug/lib 和 Release/lib。Debug 保留原有二进制，Release 使用 /MD。此前已逐一校验 Release 源码头文件与工程头文件一致。lib/cmake/ReactPhysics3D 导入元数据指向两种配置；普通 VS 构建无需 CMake。源码：https://github.com/DanielChappuis/reactphysics3d 。许可见 LICENSE.txt。预编译文件随仓库提供。
