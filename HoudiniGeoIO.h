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
    void readTetWithSurface(const std::string& filePath);
    void write(const std::string& output = "");
    
    // Setters
    void setPositions(const std::vector<double>& pos);
    
    // Getters
    std::vector<double> getPositions() const { return positions; }
    std::vector<std::vector<int>> getVert() const { return vert; }
    std::vector<int> getIndices() const { return indices; }
    std::vector<int> getSurfaceIndicies() const { return surface_indices; }
    std::vector<int> getTetIndicies() const { return tet_indices; }
    std::vector<bool> getIsSurfacePoint() const { return is_surface_point; }
    
private:
    void parseVert();
    void parsePointAttributes();
    void parsePrimAttributes();

    void parseVert_TetWithSurface();
    void parsePointAttributes_TetWithSurface();
    void parsePrimAttributes_TetWithSurface();
    
    static std::map<std::string, nlohmann::json> pairListToDict(const nlohmann::json& pairs);
    
    std::string inputPath;
    nlohmann::json raw;          // 原始JSON数据
    nlohmann::json topology;     // 拓扑数据
    nlohmann::json pointRef;     // 点引用数据
    nlohmann::json attributes;   // 属性数据
    nlohmann::json info;         // 全部info
    nlohmann::json primitives; 
    nlohmann::json pointgroups;   
    nlohmann::json primitivegroups;   
    
    // File attributes
    std::string fileVersion;
    bool hasIndex;
    int pointCount;
    int vertexCount;
    int primitiveCount; // Primitive count, e.g., number of all tetrahedra or triangles
    int surfaceCount; // Number of surface primitives, e.g., triangles
    int tetCount; // Number of tetrahedral primitives, e.g., tets
    size_t NVERT_ONE_PRIM;// Number of vertices per primitive, e.g., 3 for triangles, 4 for tet, etc.
    std::string primType; // Primitive type, e.g., "tet", "tri", etc.

    // Geometry data
    std::vector<double> positions; //一维展开的顶点位置数据，x1, y1, z1, x2, y2, z2, ...
    std::vector<int> indices; // 一维展开的顶点索引数据，t11, t12, t13, t14, t21, t22, t23, t24, ...
    std::vector<std::vector<int>> vert; // rephape之后的顶点索引数据，[[t11, t12, t13, t14], [t21, t22, t23, t24],...]

    std::vector<int> tet_indices;
    std::vector<int> surface_indices; // Surface indices for triangles, if applicable
    std::vector<bool> is_surface_point; // Whether a point is a surface point
};



//free functions for easy access and read tetrahedron vertices from indices and positions.
std::pair<std::vector<double>, std::vector<int>> EasyReadTetFromHoudini(
    const std::string& filePath
);
