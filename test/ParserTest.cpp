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
   // testTranslateVulkanComp(compCode);

   vulkanRenderAPI.DestroyAll();
   LOG.Stop();
}