# HybridPBR

项目占位文件，由 CMakeLists.txt 指定的源文件和头文件初始实现。

结构：
- include/    公共头文件
- src/        源代码（core, rendering, utils）
- assets/     资源占位

快速构建：
```powershell
mkdir build; cd build; cmake ..; cmake --build .
```

下一步建议：实现各模块的功能（窗口、输入、着色器加载等），并补充第三方库配置（OpenGL 链接在 Windows 上可能需要调整）。
