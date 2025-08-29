//
// Created by 51092 on 25-8-28.
//
#include <ShaderParse.h>
#include <fstream>
#include <sstream>
#include "Logger.h"
std::string testFilePath = "../../resources/test/testShader.caishader";
using namespace FrameWork;

void testBlockGetter(const std::string& code){
   DEBUG("测试Block Getter:");
   auto info = ShaderParse::GetCodeBlock(code, "Vertex");
   TRACE("{}", info);
}
void testFindWord(const std::string& code) {
   DEBUG("测试搜索单词: Vertex");
   auto pos = ShaderParse::FindWord(code, "Vertex", 0);
   TRACE("第一个字母位置：{}", pos);
   DEBUG("测试搜索单词: tex");
   pos = ShaderParse::FindWord(code, "tex", 0);
   TRACE("是否为npos: {}", pos == std::string::npos);
}

void testParserCode(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   DEBUG("测试 ParserCode :");
   TRACE("Vert :{}", vert);
   TRACE("Frag :{}", frag);

   DEBUG("测试 RemoveCRLF:");
   ShaderParse::RemoveCRLF(frag);
   ShaderParse::RemoveCRLF(vert);
   TRACE("Remove VERT CRLF:\n{}", vert);
   TRACE("Remove FRAG CRLF:\n{}", frag);

   ShaderParse::RemoveSpaces(vert);
   ShaderParse::RemoveSpaces(frag);
   DEBUG("Remove VERT Space:\n{}", vert);
   DEBUG("Remove FRAG Space:\n{}", frag);
}

void testSplit(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   DEBUG("测试split函数：");
   auto tokens = ShaderParse::SplitString(vert, '\n');
   for (auto& token : tokens) {
      TRACE("{}", token);
   }
}

void testGetProperty(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   DEBUG("测试GetProperty:");
   auto vertPropertiesInfo = ShaderParse::GetShaderProperties(vert);
   auto fragPropertiesInfo = ShaderParse::GetShaderProperties(frag);
   TRACE("Vert:");
   for (auto & vertbase : vertPropertiesInfo.baseProperties) {
      TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   for (auto & vertbase : vertPropertiesInfo.textureProperties){
      TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   TRACE("Frag:");
   for (auto & vertbase : fragPropertiesInfo.baseProperties) {
      TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   for (auto & vertbase : fragPropertiesInfo.textureProperties){
      TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
}

void testGetShaderInfo(const std::string& code) {
   auto info = ShaderParse::GetShaderInfo(code);
   DEBUG("测试GetShaderInfo");
}

void testShaderStateSet(const std::string& code) {
   auto stateSet = ShaderParse::GetShaderStateSet(code);
   DEBUG("测试ShaderStateSet");
}

int main(){
   LOG.Run();
   LOG.SetPrintToFile(false);
   std::ifstream testFile(testFilePath);
   if (! testFile.is_open()) {
      ERROR("Failed to open test file from: {}", testFilePath);
   }
   std::stringstream ss;
   ss << testFile.rdbuf();
   std::string code = ss.str();
   // testBlockGetter(code);
   // testFindWord(code);
   // testParserCode(code);
   // testSplit(code);
   // testGetProperty(code);
   testGetShaderInfo(code);
   // testShaderStateSet(code);
   LOG.Stop();
}