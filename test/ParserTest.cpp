//
// Created by 51092 on 25-8-28.
//
#include <ShaderParse.h>
#include <fstream>
#include <sstream>
#include "Logger.h"
#include <vulkanFrameWork.h>
std::string testFilePath = "../../resources/test/testShader.caishader";
std::string compFilePath = "../../resources/test/testComp.compshader";
using namespace FrameWork;

void testBlockGetter(const std::string& code){
   LOG_DEBUG("测试Block Getter:");
   auto info = ShaderParse::GetCodeBlock(code, "Vertex");
   LOG_TRACE("{}", info);
}
void testFindWord(const std::string& code) {
   LOG_DEBUG("测试搜索单词: Vertex");
   auto pos = ShaderParse::FindWord(code, "Vertex", 0);
   LOG_TRACE("第一个字母位置：{}", pos);
   LOG_DEBUG("测试搜索单词: tex");
   pos = ShaderParse::FindWord(code, "tex", 0);
   LOG_TRACE("是否为npos: {}", pos == std::string::npos);
}

void testParserCode(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   LOG_DEBUG("测试 ParserCode :");
   LOG_TRACE("Vert :{}", vert);
   LOG_TRACE("Frag :{}", frag);

   LOG_DEBUG("测试 RemoveCRLF:");
   ShaderParse::RemoveCRLF(frag);
   ShaderParse::RemoveCRLF(vert);
   LOG_TRACE("Remove VERT CRLF:\n{}", vert);
   LOG_TRACE("Remove FRAG CRLF:\n{}", frag);

   ShaderParse::RemoveSpaces(vert);
   ShaderParse::RemoveSpaces(frag);
   LOG_DEBUG("Remove VERT Space:\n{}", vert);
   LOG_DEBUG("Remove FRAG Space:\n{}", frag);
}

void testSplit(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   LOG_DEBUG("测试split函数：");
   auto tokens = ShaderParse::SplitString(vert, '\n');
   for (auto& token : tokens) {
      LOG_TRACE("{}", token);
   }
}

void testGetProperty(const std::string& code) {
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   LOG_DEBUG("测试GetProperty:");
   auto vertPropertiesInfo = ShaderParse::GetShaderProperties(vert);
   auto fragPropertiesInfo = ShaderParse::GetShaderProperties(frag);
   LOG_TRACE("Vert:");
   for (auto & vertbase : vertPropertiesInfo.baseProperties) {
      LOG_TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   for (auto & vertbase : vertPropertiesInfo.textureProperties){
      LOG_TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   LOG_TRACE("Frag:");
   for (auto & vertbase : fragPropertiesInfo.baseProperties) {
      LOG_TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
   for (auto & vertbase : fragPropertiesInfo.textureProperties){
      LOG_TRACE("Name : {}, Len : {}", vertbase.name, vertbase.arrayLength);
   }
}

void testGetShaderInfo(const std::string& code) {
   auto info = ShaderParse::GetShaderInfo(code);
   LOG_DEBUG("测试GetShaderInfo");
}

void testShaderStateSet(const std::string& code) {
   auto stateSet = ShaderParse::GetShaderStateSet(code);
   LOG_DEBUG("测试ShaderStateSet");
}

void testTranslate(const std::string& code) {
   auto info = ShaderParse::GetShaderInfo(code);
   LOG_DEBUG("测试Translate");
   std::string vert, frag;
   ShaderParse::ParseShaderCode(code, vert, frag);
   auto vulkanCode = ShaderParse::TranslateToVulkan(vert, info.vertProperties);
   LOG_TRACE("Vert :{}", vulkanCode);
}

void testGetShaderModule() {
   auto& resource = Resource::GetInstance();
   ShaderInfo shaderInfo;
   auto shaderModules = resource.GetShaderCaIShaderModule(
      vulkanRenderAPI.GetVulkanDevice()->logicalDevice, testFilePath, shaderInfo);
   for (auto& shaderModule : shaderModules) {
      vkDestroyShaderModule(vulkanRenderAPI.GetVulkanDevice()->logicalDevice, shaderModule.second, nullptr);
   }
}

void testCreatePipeline() {
   LOG_TRACE("Test CreatePipeline");
   uint32_t pipelineID = -1;
   auto shaderInfo = vulkanRenderAPI.CreateVulkanPipeline(
      pipelineID, testFilePath
      );
   LOG_DEBUG("会报关于没有释放descriptorSetLayout的错，因为我为了集中性将DescriptorSetLayout的管理封装给了slot，得后续处理");
};

void testGetLocalInvocation(const std::string& code) {
   auto localInvocation = ShaderParse::GetCompLocalInvocation(code);
   LOG_TRACE("x: {}, y: {}, z: {}", localInvocation.x, localInvocation.y, localInvocation.z);
}

void testGetCompShaderInfo(const std::string& code) {
   auto info = ShaderParse::GetCompShaderInfo(code);
}

void testTranslateVulkanComp(const std::string& code) {
   auto info = ShaderParse::GetCompShaderInfo(code);
   auto vulkanCode = ShaderParse::TranslateCompToVulkan(code, info);
}

int main(){
   LOG.Run();
   vulkanRenderAPI.initVulkan();
   LOG.SetPrintToFile(false);
   std::ifstream testFile(testFilePath);
   if (! testFile.is_open()) {
      LOG_ERROR("Failed to open test file from: {}", testFilePath);
   }
   std::stringstream ss;
   ss << testFile.rdbuf();
   std::string code = ss.str();

   std::ifstream compFile(compFilePath);
   if (! compFile.is_open()) {
      LOG_ERROR("Failed to open test file from: {}", testFilePath);
   }
   std::stringstream compSS;
   compSS << compFile.rdbuf();
   std::string compCode = compSS.str();
   LOG_DEBUG("{}", compCode);

   // testGetShaderInfo(code);
   // testBlockGetter(code);
   // testFindWord(code);
   // testParserCode(code);
   // testSplit(code);
   // testGetProperty(code);
   // testGetShaderInfo(code);
   // testShaderStateSet(code);
   // testTranslate(code);
   // testGetShaderInfo(code);
   // testGetShaderModule();
   // testCreatePipeline();
   // testGetLocalInvocation(compCode);
   // testGetCompShaderInfo(compCode);
   testTranslateVulkanComp(compCode);

   vulkanRenderAPI.DestroyAll();
   LOG.Stop();
}