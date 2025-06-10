#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <Eigen/Dense>
#include "json.hpp"


class HoudiniGeoIO {
public:
    HoudiniGeoIO(const std::string& input = "");
    
    void read(const std::string& filePath);
    void write(const std::string& output = "");
    
    // Setters
    void setPositions(const std::vector<float>& pos);
    
    // Getters
    std::vector<float> getPositions() const { return positions; }
    std::vector<std::vector<int>> getVert() const { return vert; }
    std::vector<int> getIndices() const { return indices; }
    
private:
    void parseVert();
    void parsePointAttributes();
    void parsePrimAttributes();
    
    static std::map<std::string, nlohmann::json> pairListToDict(const nlohmann::json& pairs);
    
    std::string inputPath;
    nlohmann::json raw;          // 原始JSON数据
    nlohmann::json topology;     // 拓扑数据
    nlohmann::json pointRef;     // 点引用数据
    nlohmann::json attributes;   // 属性数据
    
    // File attributes
    std::string fileVersion;
    bool hasIndex;
    int pointCount;
    int vertexCount;
    int primitiveCount;
    size_t NVERT_ONE_PRIM;// Number of vertices per primitive, e.g., 3 for triangles, 4 for tet, etc.
    std::string primType; // Primitive type, e.g., "tet", "tri", etc.

    // Geometry data
    std::vector<float> positions; //一维展开的顶点位置数据，x1, y1, z1, x2, y2, z2, ...
    std::vector<int> indices; // 一维展开的顶点索引数据，t11, t12, t13, t14, t21, t22, t23, t24, ...
    std::vector<std::vector<int>> vert; // rephape之后的顶点索引数据，[[t11, t12, t13, t14], [t21, t22, t23, t24],...]
};



//free functions for easy access and read tetrahedron vertices from indices and positions.
std::pair<std::vector<float>, std::vector<int>> EasyReadTetFromHoudini(
    const std::string& filePath
);
