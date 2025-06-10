#include <iostream>
#include <fstream>
#include <vector>
#include "HoudiniGeoIO.h"


// example usage
int main() {
    // 用法1：实例化一个Geo对象并读取文件（这种方法保留geo实例可以供后续输出和使用）。
    HoudiniGeoIO geo("D:/Dev/Gaia/Simulator/3rdParty/HoudiniGeoIO/two_balls_self_intersection.geo");
    auto indices1 = geo.getIndices(); // 获取顶点索引
    auto positions1 = geo.getPositions(); // 获取顶点位置

    // 用法2：使用EasyReadTetFromHoudini函数读取位置和索引，适合仅读取一次时使用。
    auto& [positions2 ,indices2] = EasyReadTetFromHoudini(
        "D:/Dev/Gaia/Simulator/3rdParty/HoudiniGeoIO/two_balls_self_intersection.geo"
    );

    // 两种选一种即可。
}
