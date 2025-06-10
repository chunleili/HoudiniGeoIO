// https://www.sidefx.com/docs/houdini/io/formats/geo.html


#include "HoudiniGeoIO.h"
#include <fstream>
#include <filesystem>
#include <iostream>  // 添加这一行
#include "HoudiniGeoIO.h"


HoudiniGeoIO::HoudiniGeoIO(const std::string& input) {
    if (!input.empty()) {
        inputPath = input;
        read(input);
    }
}

void HoudiniGeoIO::read(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    try {
        // 从文件读取JSON数据
        raw = nlohmann::json::parse(file);
        
        // 遍历键值对并设置属性
        for (size_t i = 0; i < raw.size(); i += 2) {
            const auto& name = raw[i];
            const auto& item = raw[i + 1];
            
            // 设置基本属性
            if (name == "fileversion") fileVersion = item;
            else if (name == "hasindex") hasIndex = item;
            else if (name == "pointcount") pointCount = item;
            else if (name == "vertexcount") vertexCount = item;
            else if (name == "primitivecount") primitiveCount = item;
            else if (name == "topology") topology = item;
            else if (name == "attributes") attributes = item;
        }

        // 处理拓扑结构
        topology = pairListToDict(topology);
        if (topology.find("pointref") != topology.end()) {
            pointRef = pairListToDict(topology["pointref"]);
        }

        // 处理属性
        attributes = pairListToDict(attributes);

        
        // 解析所有数据
        parseVert();
        parsePointAttributes();
        // parsePrimAttributes();

        std::cout << "Finish reading geo file: " << filePath << std::endl;
    }
    catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("JSON parsing error: " + std::string(e.what()) + 
                               "\nFile: " + filePath);
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Error reading file: " + std::string(e.what()) + 
                               "\nFile: " + filePath);
    }
}


std::map<std::string, nlohmann::json> HoudiniGeoIO::pairListToDict(const nlohmann::json& pairs) {
    std::map<std::string, nlohmann::json> result;
    for (size_t i = 0; i < pairs.size(); i += 2) {
        result[pairs[i]] = pairs[i + 1];
    }
    return result;
}

void HoudiniGeoIO::parseVert() {
    // 检查是否存在indices
    if (pointRef.find("indices") == pointRef.end()) {
        return;
    }
    
    // 检查primitivecount是否为0
    if (primitiveCount == 0) {
        return;
    }
    
    // 获取indices数据
    indices = pointRef["indices"].get<std::vector<int>>();
    // 检查indices长度是否正确
    if (indices.size() % primitiveCount != 0) {
        throw std::runtime_error("Indices size wrong");
    }
    
    // 计算每个图元的顶点数
    NVERT_ONE_PRIM = indices.size() / primitiveCount;
    if (NVERT_ONE_PRIM == 4) {
        primType = "tet";
    }
    else if (NVERT_ONE_PRIM == 3) {
        primType = "tri";
    }
    else {
        throw std::runtime_error("Unsupported primitive type: NVERT_ONE_PRIM=" + std::to_string(NVERT_ONE_PRIM)); 
    }
    
    // 重塑数组 (模拟numpy的reshape操作)
    vert.resize(primitiveCount);
    for (int i = 0; i < primitiveCount; i++) {
        vert[i].resize(NVERT_ONE_PRIM);
        for (int j = 0; j < NVERT_ONE_PRIM; j++) {
            vert[i][j] = indices[i * NVERT_ONE_PRIM + j];
        }
    }
}




void HoudiniGeoIO::parsePointAttributes() {
    // 获取 pointattributes
    if (attributes.find("pointattributes") == attributes.end()) {
        throw std::runtime_error("No pointattributes found in file");
    }
    
    const auto& pointAttributes = attributes["pointattributes"];
    if (!pointAttributes.is_array()) {
        throw std::runtime_error("pointattributes is not an array");
    }

    // 遍历所有属性
    for (size_t i = 0; i < pointAttributes.size(); i++) {
        // 获取metadata和data
        const auto& metadata = pointAttributes[i][0];
        const auto& data = pointAttributes[i][1];

        // 检查是否是P属性
        std::string attrName;
        for (size_t j = 0; j < metadata.size(); j += 2) {
            if (metadata[j] == "name") {
                attrName = metadata[j + 1];
                break;
            }
        }
        
        bool got_P = false;  // 标记是否已获取P属性
        if (attrName == "P") {
            // 找到P属性，解析位置数据
            for (size_t j = 0; j < data.size(); j += 2) {
                if (data[j] == "values") {
                    const auto& rawValues = data[j + 1][5];  // 获取 values[5]
                    
                    if (!rawValues.is_array()) {
                        throw std::runtime_error("Position data is not an array");
                    }

                    if (rawValues.size()!= pointCount) {
                        throw std::runtime_error("Position data size does not match point count");
                    }
                    
                    // 将嵌套数组展平为一维数组
                    positions.clear();
                    positions.reserve(rawValues.size() * 3);  // 预分配内存
                    
                    for (const auto& point : rawValues) {
                        if (!point.is_array() || point.size() != 3) {
                            throw std::runtime_error("Invalid position data format - expecting array of size 3");
                        }
                        positions.push_back(point[0].get<float>());
                        positions.push_back(point[1].get<float>());
                        positions.push_back(point[2].get<float>());
                    }

                    if (positions.size()!= pointCount * 3) {
                        throw std::runtime_error("Position data size does not match point count"); 
                    }

                    got_P = true;  // 标记已获取P属性
                }
            }
        }
        if (!got_P) {
            throw std::runtime_error("No P attribute found in point attributes");
        }

    }
}

void HoudiniGeoIO::parsePrimAttributes() {
    //TODO: 解析图元属性
}


void HoudiniGeoIO::write(const std::string& output) {
    std::string outputPath = output;
    if (outputPath.empty()) {
        std::filesystem::path p(inputPath);
        outputPath = p.parent_path().string() + "/" + p.stem().string() + ".geo";
    }
    
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + outputPath);
    }
    
    file << raw.dump();
    std::cout << "Finish writing geo file: " << outputPath << std::endl;
}


void HoudiniGeoIO::setPositions(const std::vector<float>& pos) {

}


//free functions for easy access and read tetrahedron vertices from indices and positions.
// It is suitable for only reading the geo once.
std::pair<std::vector<float>, std::vector<int>> EasyReadTetFromHoudini(
    const std::string& filePath
)
{
    HoudiniGeoIO reader(filePath);
    std::vector<int> indices = reader.getIndices(); // 获取顶点索引
    std::vector<float> positions = reader.getPositions(); // 获取顶点位置
    return {positions, indices};
}
