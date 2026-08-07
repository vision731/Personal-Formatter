#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

/**
 * JSP 压缩器 - 将所有非保护区内的空白字符压缩为单个空格
 * 
 * 状态机规则（严格遵循 README）：
 * 1. 普通模式：引号外，空白字符 -> 1个空格，非空白原样输出
 * 2. 字符串保护模式：引号内，全量原样输出，遇到反斜杠索引+2跳过转义字符
 * 3. 注释吞噬模式：丢弃所有字符直到闭合符
 */
class JspCompressor {
public:
    std::string compress(const std::string& input) {
        std::ostringstream output;
        size_t i = 0;
        const size_t len = input.length();
        
        // 状态定义
        enum State {
            NORMAL,          // 普通模式
            IN_DOUBLE_QUOTE, // 双引号内
            IN_SINGLE_QUOTE, // 单引号内
            IN_BACKTICK,     // 反引号内（兜底）
            IN_COMMENT_HTML, // HTML 注释 <!-- -->
            IN_COMMENT_JSP,  // JSP 注释 <%-- --%>
            IN_COMMENT_JS_SINGLE, // JS 单行注释 //
            IN_COMMENT_JS_MULTI   // JS 多行注释 /* */
        };
        
        State state = NORMAL;
        char quoteChar = '\0'; // 用于区分引号类型
        
        auto isWhitespace = [](char c) -> bool {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        };
        
        // 辅助：判断是否是注释起始
        auto startsWith = [&](size_t pos, const std::string& pattern) -> bool {
            if (pos + pattern.length() > len) return false;
            for (size_t j = 0; j < pattern.length(); ++j) {
                if (input[pos + j] != pattern[j]) return false;
            }
            return true;
        };
        
        // 辅助：吞噬直到匹配指定结束符，返回吞噬后的位置
        auto skipUntil = [&](size_t pos, const std::string& endPattern) -> size_t {
            size_t endPos = input.find(endPattern, pos);
            if (endPos == std::string::npos) {
                return len; // 文件结束，直接返回末尾
            }
            return endPos + endPattern.length();
        };
        
        bool lastWasSpace = false; // 用于压缩连续空白为单个空格
        
        while (i < len) {
            char c = input[i];
            
            switch (state) {
                // ========== 普通模式 ==========
                case NORMAL: {
                    // 先检查是否进入注释（普通模式下优先级最高）
                    if (startsWith(i, "<!--")) {
                        state = IN_COMMENT_HTML;
                        i += 4; // 跳过 <!--
                        lastWasSpace = false; // 注释丢弃后重置空格标记
                        continue;
                    }
                    if (startsWith(i, "<%--")) {
                        state = IN_COMMENT_JSP;
                        i += 4; // 跳过 <%--
                        lastWasSpace = false;
                        continue;
                    }
                    if (startsWith(i, "//")) {
                        state = IN_COMMENT_JS_SINGLE;
                        i += 2;
                        lastWasSpace = false;
                        continue;
                    }
                    if (startsWith(i, "/*")) {
                        state = IN_COMMENT_JS_MULTI;
                        i += 2;
                        lastWasSpace = false;
                        continue;
                    }
                    
                    // 检查是否进入字符串保护模式
                    if (c == '"') {
                        state = IN_DOUBLE_QUOTE;
                        output << c;
                        i++;
                        continue;
                    }
                    if (c == '\'') {
                        state = IN_SINGLE_QUOTE;
                        output << c;
                        i++;
                        continue;
                    }
                    if (c == '`') { // 兜底：虽然约定不用但支持
                        state = IN_BACKTICK;
                        output << c;
                        i++;
                        continue;
                    }
                    
                    // 普通字符处理：空白压缩
                    if (isWhitespace(c)) {
                        if (!lastWasSpace) {
                            output << ' ';
                            lastWasSpace = true;
                        }
                    } else {
                        output << c;
                        lastWasSpace = false;
                    }
                    i++;
                    break;
                }
                
                // ========== 双引号保护模式 ==========
                case IN_DOUBLE_QUOTE: {
                    if (c == '\\') {
                        // 遇到反斜杠：原样输出反斜杠和下一个字符，索引+2
                        output << c;
                        if (i + 1 < len) {
                            output << input[i + 1];
                            i += 2;
                        } else {
                            i++;
                        }
                        continue;
                    }
                    if (c == '"') {
                        output << c;
                        state = NORMAL;
                        i++;
                        continue;
                    }
                    output << c;
                    i++;
                    break;
                }
                
                // ========== 单引号保护模式 ==========
                case IN_SINGLE_QUOTE: {
                    if (c == '\\') {
                        output << c;
                        if (i + 1 < len) {
                            output << input[i + 1];
                            i += 2;
                        } else {
                            i++;
                        }
                        continue;
                    }
                    if (c == '\'') {
                        output << c;
                        state = NORMAL;
                        i++;
                        continue;
                    }
                    output << c;
                    i++;
                    break;
                }
                
                // ========== 反引号保护模式（兜底） ==========
                case IN_BACKTICK: {
                    if (c == '\\') {
                        output << c;
                        if (i + 1 < len) {
                            output << input[i + 1];
                            i += 2;
                        } else {
                            i++;
                        }
                        continue;
                    }
                    if (c == '`') {
                        output << c;
                        state = NORMAL;
                        i++;
                        continue;
                    }
                    output << c;
                    i++;
                    break;
                }
                
                // ========== 注释吞噬模式 ==========
                case IN_COMMENT_HTML: {
                    // 查找 -->，找到后跳过后继续
                    size_t endPos = input.find("-->", i);
                    if (endPos == std::string::npos) {
                        i = len; // 没有闭合，直接吞到末尾
                    } else {
                        i = endPos + 3; // 跳过 -->
                    }
                    state = NORMAL;
                    lastWasSpace = false; // 注释后重置空格标记
                    break;
                }
                
                case IN_COMMENT_JSP: {
                    size_t endPos = input.find("--%>", i);
                    if (endPos == std::string::npos) {
                        i = len;
                    } else {
                        i = endPos + 4; // 跳过 --%>
                    }
                    state = NORMAL;
                    lastWasSpace = false;
                    break;
                }
                
                case IN_COMMENT_JS_SINGLE: {
                    // 单行注释：找到 \n 或 \r\n 或文件末尾
                    while (i < len && input[i] != '\n' && input[i] != '\r') {
                        i++;
                    }
                    // 如果是 \r\n，需要额外跳过 \n
                    if (i < len && input[i] == '\r' && i + 1 < len && input[i + 1] == '\n') {
                        i += 2;
                    } else if (i < len) {
                        i++; // 跳过 \n 或 \r
                    }
                    state = NORMAL;
                    lastWasSpace = false;
                    break;
                }
                
                case IN_COMMENT_JS_MULTI: {
                    size_t endPos = input.find("*/", i);
                    if (endPos == std::string::npos) {
                        i = len;
                    } else {
                        i = endPos + 2; // 跳过 */
                    }
                    state = NORMAL;
                    lastWasSpace = false;
                    break;
                }
            }
        }
        
        // 后处理：去除首尾可能多余的空格，但通常不需要，保留原样
        return output.str();
    }
};

// ========== 使用示例 ==========
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "用法: " << argv[0] << " <输入JSP文件路径> <输出JSP文件路径>" << std::endl;
        return 1;
    }
    
    const char* inputPath  = argv[1];
    const char* outputPath = argv[2];
    
    // 二进制模式：保留原始字节（CRLF 不被系统转译，UTF-8 不被破坏）
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "错误: 无法打开输入文件 " << inputPath << std::endl;
        return 1;
    }
    
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();
    
    JspCompressor compressor;
    std::string result = compressor.compress(buffer.str());
    
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "错误: 无法打开输出文件 " << outputPath << std::endl;
        return 1;
    }
    outFile << result;
    outFile.close();
    
    std::cout << "压缩完成: " << inputPath << " -> " << outputPath << std::endl;
    return 0;
}