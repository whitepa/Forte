#include <gtest/gtest.h>
#include "FString.h"
#include "LogManager.h"

using namespace Forte;

LogManager logManager;

class FStringTest : public ::testing::Test
{
protected:
    static void SetUpTestCase() 
    {
        logManager.BeginLogging("//stdout");
        logManager.SetLogMask("//stdout", HLOG_ALL);
        hlog(HLOG_DEBUG, "Starting test...");
    }

    static void TearDownTestCase() 
    {
        logManager.EndLogging("//stdout");
    }

};

TEST_F(FStringTest, FStringTrimTest)
{
    FString s, d;

    s = "abcd";
    d = s; d.Trim();
    ASSERT_EQ(d, "abcd"); 

    s = " abcd   ";
    d = s; d.Trim();
    ASSERT_EQ(d, "abcd"); 

    s = " ab cd";
    d = s; d.Trim();
    ASSERT_EQ(d, "ab cd"); 

    s = "          ";
    d = s; d.Trim();
    ASSERT_EQ(d, ""); 

    s = "";
    d = s; d.Trim();
    ASSERT_EQ(d, ""); 

    s = " abcdx";
    d = s; d.Trim("x");
    ASSERT_EQ(d, " abcd"); 


}

TEST_F(FStringTest, FStringReplace)
{
    FString s, d;
    
    s = "quite";
    d = s; d.Replace("te", "et");
    ASSERT_EQ(d, "quiet");

    s = "quite";
    d = s; d.Replace("master", "slave");
    ASSERT_EQ(d, "quite");

    s = "quiet";
    d = s; d.Replace("quiet", "loud");
    ASSERT_EQ(d, "loud");

    s = "quiet";
    d = s; d.Replace("qu", "di");
    ASSERT_EQ(d, "diiet");

    s = "";
    d = s; d.Replace("aa", "bb");
    ASSERT_EQ(d, "");
}

TEST_F(FStringTest, FStringLineSplit)
{
    FString s;
    std::vector<FString> lines;
    
    s = "ab\ncd\n ef";
    s.LineSplit(lines);
    ASSERT_EQ(lines.size(), 3);
    ASSERT_EQ(lines[0], "ab");
    ASSERT_EQ(lines[1], "cd");
    ASSERT_EQ(lines[2], " ef");

    s = "ab\ncd\n ef";
    s.LineSplit(lines, true);
    ASSERT_EQ(lines.size(), 3);
    ASSERT_EQ(lines[0], "ab");
    ASSERT_EQ(lines[1], "cd");
    ASSERT_EQ(lines[2], "ef");

    s = "\n\n\n";
    s.LineSplit(lines);
    ASSERT_EQ(lines.size(), 4);
    ASSERT_EQ(lines[0], "");
    ASSERT_EQ(lines[1], "");
    ASSERT_EQ(lines[2], "");
    ASSERT_EQ(lines[3], "");
}

TEST_F(FStringTest, ExplodeFStringTest)
{
    FString s;
    std::vector<FString> shards;

    s = "this is a test";
    s.Explode(" ", shards);
    ASSERT_EQ(shards.size(), 4);
    ASSERT_EQ(shards[1], "is");

    s = "thisXXXisXXXaXXXtest";
    s.Explode("XXX", shards);
    ASSERT_EQ(shards.size(), 4);
    ASSERT_EQ(shards[1], "is");

    s = "bhello    aworlda";
    s.Explode("    ", shards, true, "ab");
    ASSERT_EQ(shards.size(), 2);
    ASSERT_EQ(shards[1], "world");
}

TEST_F(FStringTest, ImplodeExplodeBinaryFStringTest)
{
    FString s;
    std::vector<FString> components;

    components.push_back("this is first vector");
    components.push_back("this is second vector");
    components.push_back("");
    components.push_back("this is third vector");

    s.ImplodeBinary(components);
    std::vector<FString> componentsTest;

    s.ExplodeBinary(componentsTest);

    ASSERT_EQ(components.size(), componentsTest.size());

    for (int i=0; i < components.size(); i++)
    {
        ASSERT_STREQ( components[i], componentsTest[i] );
    }
}

TEST_F(FStringTest, TokenizeFStringTest)
{
    FString s, d;
    std::vector<FString> tokens;

    s = "this is a test";
    d = s; 
    d.Tokenize(" ", tokens);
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[1], "is");

    s = "1this234is56a78test90";
    d = s; 
    d.Tokenize("1234567890", tokens);
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[3], "test");

    s = " this is   a    test  ";
    d = s; 
    d.Tokenize(" ", tokens);
    ASSERT_EQ(tokens.size(), 4);
    ASSERT_EQ(tokens[0], "this");
    ASSERT_EQ(tokens[1], "is");

    s = " this is   a    test  ";
    d = s; 
    d.Tokenize("Z12345\n", tokens);
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0], " this is   a    test  ");
    
    s = " this is    a     test  ";
    d = s;
    d.Tokenize(" ", tokens, 2);
    ASSERT_EQ(tokens.size(), 2);
    ASSERT_EQ(tokens[0], "this");
    ASSERT_EQ(tokens[1], "is    a     test  ");

    s = " this is    a     test  ";
    d = s;
    d.Tokenize(" ", tokens, 1);
    ASSERT_EQ(tokens.size(), 1);
    ASSERT_EQ(tokens[0], "this is    a     test  ");
}

TEST_F(FStringTest, OFormat)
{
    FString format("test no params");
    std::vector<FString> fParams;
    std::vector<std::string> sParams;

    ASSERT_STREQ(FString(FStringFO(), format, fParams), format);
    ASSERT_STREQ(FString(FStringFO(), static_cast<std::string>(format), sParams), format);

    format = "test %@%@ blah%@";
    fParams.push_back("1");
    fParams.push_back("2");
    fParams.push_back("3");
    sParams.push_back("1");
    sParams.push_back("2");
    sParams.push_back("3");

    ASSERT_STREQ(FString(FStringFO(), format, fParams), FString("test 12 blah3"));
    ASSERT_STREQ(FString(FStringFO(), static_cast<std::string>(format), sParams),
            FString("test 12 blah3"));
}
