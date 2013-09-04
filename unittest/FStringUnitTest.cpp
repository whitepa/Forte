#include <gtest/gtest.h>
#include "FString.h"
#include "LogManager.h"

using namespace Forte;
using namespace boost;

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

TEST_F(FStringTest, ConstructFromInt64_t)
{
    int64_t i(1232131231232);
    FString s(i);
    ASSERT_EQ(s, "1232131231232");
}

TEST_F(FStringTest, FStringStripNonMatchingCharsTest)
{
    FString s, d;

    s = ",a.b c<d;";
    s.StripNonMatchingChars(",a.b c<d;");
    ASSERT_EQ(s, ",a.b c<d;");

    s.StripNonMatchingChars(",abc<d;");
    ASSERT_EQ(s, ",abc<d;");

    s.StripNonMatchingChars(",cba<d;");
    ASSERT_EQ(s, ",abc<d;");

    s.StripNonMatchingChars("abc<d;");
    ASSERT_EQ(s, "abc<d;");

    s.StripNonMatchingChars("ad");
    ASSERT_EQ(s, "ad");
}

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

TEST_F(FStringTest, FStringRightLeftStringChop)
{
    FString s, d;

    s = "abcd.efg.hijk.lmnop";
    d = s; d.RightString(".");
    ASSERT_EQ(d, "lmnop");

    d = s; d.LeftString(".");
    ASSERT_EQ(d, "abcd");

    d = s; d.ChopLeft(".");
    ASSERT_EQ(d, "efg.hijk.lmnop");

    d = s; d.ChopRight(".");
    ASSERT_EQ(d, "abcd.efg.hijk");

    d = s; d.ChopRight(".").ChopLeft(".");
    ASSERT_EQ(d, "efg.hijk");

    d = "."; d.ChopLeft(".");
    ASSERT_EQ(d, "");

    d = ""; d.ChopLeft(".");
    ASSERT_EQ(d, "");

    d = "......"; d.ChopLeft(".");
    ASSERT_EQ(d, ".....");

    hlog(HLOG_DEBUG, "ChopLeft Insanity");
    d = s; d.ChopLeft(".").ChopLeft(".").ChopLeft(".").ChopLeft(".").ChopLeft(".").ChopLeft(".");
    ASSERT_EQ(d, "lmnop");

    d = s;
    ASSERT_TRUE(d.IsDelimited("."));

    d = "no delimiter";
    ASSERT_FALSE(d.IsDelimited("."));

try {
    hlog(HLOG_DEBUG, "ChopRight Insanity");
    d = s; d.ChopRight(".").ChopRight(".").ChopRight(".").ChopRight(".").ChopRight(".").ChopRight(".");
    ASSERT_EQ(d, "abcd");
} catch (std::exception &e) { hlog(HLOG_DEBUG, "chopright exception: (%s)", e.what()); }

    hlog(HLOG_DEBUG, "RightString Insanity");
    d = s; d.RightString(".").RightString(".").RightString(".").RightString(".").RightString(".").RightString(".");
    ASSERT_EQ(d, "lmnop");

    hlog(HLOG_DEBUG, "LeftString Insanity");
    d = s; d.LeftString(".").LeftString(".").LeftString(".").LeftString(".").LeftString(".").LeftString(".");
    ASSERT_EQ(d, "abcd");

    hlog(HLOG_DEBUG, "non-. delimiter");
    d = "dev.scale.slot5p4";
    ASSERT_NE(d.LeftString("v"), "dev");

    d = "/dev/scale/slot5p4";
    d.RightString("'t'");
    hlog(HLOG_DEBUG, "d value: (%s)", d.c_str());
    ASSERT_EQ(d, "5p4");

    d.LeftString("p");
    ASSERT_EQ(d, "5");

    d = "/dev/scale/slot5p4";
    ASSERT_EQ(d.RightString("t").LeftString("p"), "5");
}

TEST_F(FStringTest, Regexplode)
{
    FString foo("abcdefg");
    std::vector<FString> bar;
    ASSERT_TRUE(foo.RegexMatch("b(.)de(.)", bar));
    ASSERT_EQ(bar[0], "c");
    ASSERT_EQ(bar[1], "f");

    bar.clear();
    ASSERT_FALSE(foo.RegexMatch("hijklmnop", bar));

    FString baz("abc123def456");
    ASSERT_TRUE(baz.RegexMatch("abc(\\d\\d\\d)def", bar));
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

TEST_F(FStringTest, DelimitedListToSet_FString)
{
    FString s;
    std::set<FString> shards;
    std::set<FString> desiredShards;

    desiredShards.insert("this");
    desiredShards.insert("is");
    desiredShards.insert("a");
    desiredShards.insert("test");

    s = "this is a test";
    s.DelimitedListToSet(" ", shards);
    EXPECT_TRUE(desiredShards == shards);

    s = "thisXXXisXXXaXXXtest";
    shards.clear();
    s.DelimitedListToSet("XXX", shards);
    EXPECT_TRUE(desiredShards == shards);

    shards.clear();
    s = "xxxthisx,is ,a, testxxxy";
    s.DelimitedListToSet(",", shards, true, " xy");
    EXPECT_TRUE(desiredShards == shards);
}

TEST_F(FStringTest, DelimitedListToSet_StdString)
{
    FString s;
    std::set<std::string> shards;
    std::set<std::string> desiredShards;

    desiredShards.insert("this");
    desiredShards.insert("is");
    desiredShards.insert("a");
    desiredShards.insert("test");

    s = "this is a test";
    s.DelimitedListToSet(" ", shards);
    EXPECT_TRUE(desiredShards == shards);

    s = "thisXXXisXXXaXXXtest";
    s.DelimitedListToSet("XXX", shards);
    EXPECT_TRUE(desiredShards == shards);

    s = "xxxthisx,is ,a, testxxxy";
    s.DelimitedListToSet(",", shards, true, " xy");
    EXPECT_TRUE(desiredShards == shards);
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

    for (int i=0; i < static_cast<int>(components.size()); i++)
    {
        ASSERT_STREQ( components[i], componentsTest[i] );
    }
}

TEST_F(FStringTest, ImplodeToNull)
{
    std::vector<FString> components;

    components.push_back("a");
    components.push_back("b");
    components.push_back("c");
    ASSERT_STREQ( FString().Implode("-", components), "a-b-c" );

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

TEST_F(FStringTest, HexDump)
{
    FString binary;
    FString expected;
    EXPECT_EQ(expected,binary.HexDump());

    char buf[32];
    memset(buf, 0, sizeof(buf));

    // One octet
    binary.assign(buf, 1);
    EXPECT_STREQ(FString("00 "), binary.HexDump());

    // Two octets
    binary = "AB";
    EXPECT_STREQ(FString("41 42 "), binary.HexDump());

    // 16 octets
    binary = "abcdefghijklmnop";
    EXPECT_STREQ(FString("61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F 70 "), binary.HexDump());

    // Unsigned char needed
    memset(buf, 0xff, sizeof(buf));
    binary.assign(buf, 10);
    EXPECT_STREQ(FString("FF FF FF FF FF FF FF FF FF FF "), binary.HexDump());


}
TEST_F(FStringTest, ConstructFromVectorOfOptional)
{
    std::vector<boost::optional<uint32_t> > vec;
    vec.resize(10);
    EXPECT_STREQ(FString("_,_,_,_,_,_,_,_,_,_"), FString(vec));
    vec[0] = 0;
    vec[2] = 2;
    vec[4] = 4;
    vec[6] = 6;
    vec[8] = 8;
    EXPECT_STREQ(FString("0,_,2,_,4,_,6,_,8,_"), FString(vec));
}
