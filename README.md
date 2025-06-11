一个简单的读取Houdini .geo文件的c++代码脚本。

（在当前文件夹）

编译：build.bat

运行：run.bat

调试：用builddebug.bat编译后，用.vscode/launch.json配置在vscode下直接F5即可。


用法详见main.cpp

```c++
// 用法1：实例化一个Geo对象并读取文件（这种方法保留geo实例可以供后续输出和使用）。
HoudiniGeoIO geo("D:/Dev/Gaia/Simulator/3rdParty/HoudiniGeoIO/two_balls_self_intersection.geo");
auto indices1 = geo.getIndices(); // 获取顶点索引
auto positions1 = geo.getPositions(); // 获取顶点位置

// 用法2：使用EasyReadTetFromHoudini函数读取位置和索引，适合仅读取一次时使用。
auto& [positions2 ,indices2] = EasyReadTetFromHoudini(
    "D:/Dev/Gaia/Simulator/3rdParty/HoudiniGeoIO/two_balls_self_intersection.geo"
);

// 两种选一种即可。
```

## 集成到自己的项目
利用HoudiniGeoIO/cmake/HoudiniGeoIO-config.cmake文件可以将HoudiniGeoIO作为一个模块集成到自己的项目中。
```
find_package(HoudiniGeoIO REQUIRED PATHS /path/to/HoudiniGeoIO)
SET (YOUR_SRCS 
	${YOUR_SRCS}
	${HoudiniGeoIO_SRCS}
)
add_executable(YouApplication 
	${YOUR_SRCS}
)
target_include_directories(YouApplication PRIVATE ${HoudiniGeoIO_INCLUDE_DIR})
```