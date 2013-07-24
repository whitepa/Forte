#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LogManager.h"
#include "XMLDoc.h"
#include "XMLNode.cpp"

using namespace Forte;
using namespace boost;

LogManager logManager;

class XMLUnitTest : public ::testing::Test
{
public:
    static void SetUpTestCase() {
        logManager.SetLogMask("//stdout", HLOG_ALL);
        logManager.BeginLogging("//stdout");
    }
};

TEST_F(XMLUnitTest, XPathTest)
{
    const char * xmlBuffer =
            "<capabilities>\n"
            "  <host>\n"
            "    <uuid>534d4349-0002-3290-2500-32902500ff54</uuid>\n"
            "    <cpu>\n"
            "      <arch>x86_64</arch>\n"
            "      <model>Nehalem</model>\n"
            "      <vendor>Intel</vendor>\n"
            "      <topology sockets='1' cores='4' threads='2'/>\n"
            "      <feature name='rdtscp'/>\n"
            "      <feature name='xtpr'/>\n"
            "      <feature name='tm2'/>\n"
            "      <feature name='est'/>\n"
            "      <feature name='vmx'/>\n"
            "      <feature name='ds_cpl'/>\n"
            "      <feature name='monitor'/>\n"
            "      <feature name='pbe'/>\n"
            "      <feature name='tm'/>\n"
            "      <feature name='ht'/>\n"
            "      <feature name='ss'/>\n"
            "      <feature name='acpi'/>\n"
            "      <feature name='ds'/>\n"
            "      <feature name='vme'/>\n"
            "    </cpu>\n"
            "  </host>\n"
            "</capabilities>\n";

    XMLDoc doc(xmlBuffer);

    std::vector<XMLNode> nodes;
    doc.GetRootNode().Find(nodes, "/capabilities/host/uuid");
    ASSERT_EQ(1, nodes.size());
    ASSERT_STREQ("534d4349-0002-3290-2500-32902500ff54", nodes.at(0).GetText());

    FString result_14;
    doc.GetRootNode().XPathStr(result_14, "count(//feature)");
    ASSERT_STREQ("14", result_14);

    FString result_true;
    doc.GetRootNode().XPathStr(result_true, "count(//feature) = 14");
    ASSERT_STREQ("true", result_true);

    FString result_Nehalem;
    doc.GetRootNode().XPathStr(result_Nehalem, "//model/text()");
    ASSERT_STREQ("Nehalem", result_Nehalem);

    FString result_est;
    doc.GetRootNode().XPathStr(result_est, "//feature[@name='est']/@name");
    ASSERT_STREQ("est", result_est);

    FString result_Nehalemest;
    doc.GetRootNode().XPathStr(result_Nehalemest, "//model/text()|//feature[@name='est']/@name");
    ASSERT_STREQ("Nehalem\nest", result_Nehalemest);

    result_Nehalemest.clear();
    doc.GetRootNode().XPathStr(result_Nehalemest, "//feature[@name='est']/@name|//model/text()");
    ASSERT_STREQ("Nehalem\nest", result_Nehalemest);
}
