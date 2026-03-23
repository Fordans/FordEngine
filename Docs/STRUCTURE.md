# FordEngine 代码库结构

## 目录结构

```
FordEngine/
├── CMakeLists.txt              # 根配置
├── ThirdParty/                 # 第三方依赖 (spdlog, glfw, imgui)
└── Engine/
    ├── CMakeLists.txt          # FordEngine 库
    ├── Editor/
    │   └── CMakeLists.txt      # FordEditor 可执行
    ├── include/FDE/            # 公共头文件
    │   ├── Core/               # Application, Log
    │   ├── Editor/             # EditorApplication
    │   ├── Runtime/            # RuntimeApplication, RuntimeSession, debug ImGui layer
    │   ├── ImGui/              # ImGuiContext
    │   ├── Window/             # Window
    │   ├── Export.hpp
    │   └── FDE.hpp
    └── src/
        ├── Core/
        ├── Editor/             # EditorApplication + main
        ├── Runtime/            # runtime module sources
        ├── RuntimeApp/         # FordRuntime main.cpp (FordRuntime target in Engine/RuntimeApp/)
        ├── ImGui/
        └── Window/
```

## 设计说明

- **Engine**: 核心库
- **Editor**: 编辑器可执行目标，依赖 Engine 库
