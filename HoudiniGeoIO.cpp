// https://www.sidefx.com/docs/houdini/io/formats/geo.html


#include "HoudiniGeoIO.h"
#include <fstream>
#include <filesystem>
#include <iostream>  // 添加这一行
#include <regex>     // 添加这一行以支持 std::smatch
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
    bool got_P = false;  // 标记是否已获取P属性

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
                        positions.push_back(point[0].get<double>());
                        positions.push_back(point[1].get<double>());
                        positions.push_back(point[2].get<double>());
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
    std::cout << "Parsed point attributes successfully." << std::endl;
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


void HoudiniGeoIO::setPositions(const std::vector<double>& pos) {

}

//free functions for easy access and read tetrahedron vertices from indices and positions.
// It is suitable for only reading the geo once.
std::pair<std::vector<double>, std::vector<int>> EasyReadTetFromHoudini(
    const std::string& filePath
)
{
    HoudiniGeoIO reader(filePath);
    std::vector<int> indices = reader.getIndices(); // 获取顶点索引
    std::vector<double> positions = reader.getPositions(); // 获取顶点位置
    return {positions, indices};
}

void HoudiniGeoIO::readTetWithSurface(const std::string& filePath) {
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
            else if (name == "primitives") primitives = item;
            else if (name == "pointgroups") {
                pointgroups = item;
            }
            else if (name == "primitivegroups") {
                primitivegroups = item;
            }

            else if (name == "info") {
                info = item;
                if (info.contains("primcount_summary")) {
                    const auto& summary = info["primcount_summary"];
                    if (summary.is_string()) {
                        std::string summaryStr = summary.get<std::string>();
                        std::istringstream ss(summaryStr);
                        std::string line;
                        while (std::getline(ss, line)) {
                            // Remove leading/trailing whitespace
                            line.erase(0, line.find_first_not_of(" \t\n\r"));
                            line.erase(line.find_last_not_of(" \t\n\r") + 1);
                            if (line.empty()) continue;
                            // Find number and type
                            std::smatch match;
                            std::regex re(R"((\d[\d,]*)\s+([A-Za-z]+))");
                            if (std::regex_search(line, match, re)) {
                                int count = std::stoi(std::regex_replace(match[1].str(), std::regex(","), ""));
                                std::string primType = match[2];
                                if (primType == "Polygons" || primType == "Polygon") {
                                    surfaceCount = count;
                                } else if (primType == "Tetrahedrons" || primType == "Tetrahedron" || primType == "Tet") {
                                    tetCount = count;
                                }
                            }
                        }
                    }
                }
            }
        }
        // 处理拓扑结构
        topology = pairListToDict(topology);
        if (topology.find("pointref") != topology.end()) {
            pointRef = pairListToDict(topology["pointref"]);
        }

        // 处理属性
        attributes = pairListToDict(attributes);

        
        // 解析所有数据
        parseVert_TetWithSurface();
        std::cout << "Parsing point attributes for TetWithSurface..." << std::endl;
        parsePointAttributes_TetWithSurface();
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



void HoudiniGeoIO::parseVert_TetWithSurface() {
    // 检查是否存在indices
    if (pointRef.find("indices") == pointRef.end()) {
        return;
    }

    // 检查primitivecount是否为0
    if (primitiveCount == 0) {
        return;
    }

    // 检查primitivecount总数
    if (primitiveCount != surfaceCount + tetCount) {
        return;
    }

    // 获取indices数据
    indices = pointRef["indices"].get<std::vector<int>>();
    
    /* version1 begin: using primitivegroups to split surfaces and tets
    ////////////////////////////////////////////////////////////////////
    // // 获取primitivegroups中的surface_triangles的boolRLE
    // if (primitivegroups.is_null() || !primitivegroups.is_array()) {
    //     throw std::runtime_error("primitivegroups missing or not array");
    // }
    
    // nlohmann::json surface_triangles_group;
    // bool found_surface_triangles = false;
    // for (const auto& group : primitivegroups) {
    //     if (group.is_array() && group[0].is_array()) {
    //         if (group[0][0] == "name" && group[0][1] == "surface_triangles") {
    //             surface_triangles_group = group[1];
    //             found_surface_triangles = true;
    //             break;
    //         }
    //         if (found_surface_triangles) break;
    //     }
    // }

    // if (surface_triangles_group.is_null()) {
    //     throw std::runtime_error("surface_triangles group not found in primitivegroups");
    // }

    // // if (surface_triangles_group[1][1].find("boolRLE")== surface_triangles_group[1][1].end()) {
    // //     std::cerr << "Warning: surface_triangles group missing boolRLE, using default all false." << std::endl;
    // //     throw std::runtime_error("surface_triangles group missing boolRLE");
    // // }
    // std::vector<int> boolRLE = surface_triangles_group[1][1][1].get<std::vector<int>>();
    // // 解码boolRLE为每个primitive的bool
    // std::vector<bool> is_surface_primitive(primitiveCount, false);
    // int currentIndex = 0;
    // for (size_t i = 0; i < boolRLE.size(); i+=2) {
    //     for (int j = 0; j < boolRLE[i]; ++j) {
    //         is_surface_primitive[currentIndex+j] = boolRLE[i+1];
    //     }
    //     currentIndex += boolRLE[i];
    // }
    // if (is_surface_primitive.size() != primitiveCount) {
    //     throw std::runtime_error("Decoded boolRLE size does not match primitiveCount");
    // }
    
    // // 分割indices
    // surface_indices.clear();
    // tet_indices.clear();
    // size_t idx = 0;
    // for (int i = 0; i < primitiveCount; ++i) {
    //     if (is_surface_primitive[i]) {
    //         // surface triangle
    //         for (size_t j = 0; j < 3; ++j) {
    //             surface_indices.push_back(indices[idx + j]);
    //         }
    //         idx += 3;
    //     } else {
    //         // tet
    //         for (size_t j = 0; j < 4; ++j) {
    //             tet_indices.push_back(indices[idx + j]);
    //         }
    //         idx += 4;
    //     }
    // }
    // if (idx != indices.size() || surface_indices.size() != surfaceCount * 3 || tet_indices.size() != tetCount * 4) {
    //     throw std::runtime_error("Index size mismatch after splitting surface and tet indices");
    // }
    // }
    ////////////////////////////////////////////////////////////////////
    // version 1 end */ 

    /* version2 begin: using primitives to split*/
    surface_indices.clear();
    tet_indices.clear();

    size_t idx = 0;
    for (const auto& prim_run : primitives) {

        if (!prim_run.is_array() || prim_run.size() < 2) continue;
        const auto& type_info = prim_run[0];
        const auto& attribs = prim_run[1];
        if (!type_info.is_array() || type_info.size() < 2) continue;
        std::string type = type_info[1].get<std::string>();

        int nprimitives = 0;
        // Try to get nprimitives from attribs
        if (attribs.is_array()) {
            for (size_t i = 0; i + 1 < attribs.size(); i += 2) {
                if (attribs[i] == "nprimitives") {
                    nprimitives = attribs[i + 1].get<int>();
                    std::cout<< "nprimitives: " << nprimitives << std::endl;
                }
                // Some Polygon_run have nvertices_rle, e.g. [3, 2000]
                // if (attribs[i] == "nvertices_rle") {
                //     const auto& rle = attribs[i + 1];
                //     if (rle.is_array() && rle.size() == 2 && rle[0] == 3) {
                //         nprimitives = rle[1].get<int>();
                //         std::cout<< "nprimitives: " << nprimitives << std::endl;

                //     }
                // }
            }
        }

        if (type == "Tetrahedron_run") {
            for (int i = 0; i < nprimitives; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (idx < indices.size())
                        tet_indices.push_back(indices[idx++]);
                }
            }
        } else if (type == "Polygon_run") {
            for (int i = 0; i < nprimitives; ++i) {
                for (int j = 0; j < 3; ++j) {
                    if (idx < indices.size())
                        surface_indices.push_back(indices[idx++]);
                }
            }
        }
    }
    // Optionally, check if all indices are consumed
    if (idx != indices.size() || surface_indices.size() != surfaceCount * 3 || tet_indices.size() != tetCount * 4) {
        std::cerr << "Warning: Not all indices consumed in parseVert_TetWithSurface. idx=" << idx << ", total=" << indices.size() << std::endl;
    }
}




void HoudiniGeoIO::parsePointAttributes_TetWithSurface() {
    // 获取 pointattributes
    if (attributes.find("pointattributes") == attributes.end()) {
        throw std::runtime_error("No pointattributes found in file");
    }
    
    const auto& pointAttributes = attributes["pointattributes"];
    if (!pointAttributes.is_array()) {
        throw std::runtime_error("pointattributes is not an array");
    }

    bool got_P = false;  // 标记是否已获取P属性
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
                        positions.push_back(point[0].get<double>());
                        positions.push_back(point[1].get<double>());
                        positions.push_back(point[2].get<double>());
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

    // 解析 surface_points pointgroup 的 boolRLE，填充 is_surface_point
    is_surface_point.clear();
    if (!pointgroups.is_null() && pointgroups.is_array()) {
        nlohmann::json surface_points_group;
        for (const auto& group : pointgroups) {
            if (group.is_array() && group[0].is_array()) {
                if (group[0][0] == "name" && group[0][1] == "surface_points") {
                    surface_points_group = group[1];
                    break;
                }
            }
        }
 
        if (!surface_points_group.is_null()) {
            std::vector<int> boolRLE = surface_points_group[1][1][1].get<std::vector<int>>();
            for (size_t i = 0; i < boolRLE.size(); i+=2) {
                for (int j = 0; j < boolRLE[i]; ++j) {
                    is_surface_point.push_back(boolRLE[i+1]);
                }
            }
            if (is_surface_point.size() != pointCount) {
                throw std::runtime_error("Decoded surface_points boolRLE size does not match pointCount");
            }
        }
    }
}
