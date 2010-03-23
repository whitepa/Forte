#include "Forte.h"

// main
int main(int argc, char *argv[])
{
    FString regularString, controlCharString, stmp;
    bool all_pass = true;

    regularString = "regularString";


    XMLDoc doc;
    XMLNode root = doc.createDocument("root");


    /********************basic test*********************/
    printf("regularString: %s\n", regularString.c_str());
    XMLNode nodeRegular(regularString, root);

    printf("xmlNode: %s\n", doc.toString().c_str());

    nodeRegular.setProp(regularString, regularString);

    if (nodeRegular.getProp(regularString) != regularString)
    {
	printf("FAIL: did not get value back we set\n");
	printf("expected %s, got %s\n",
	       regularString.c_str(),
	       nodeRegular.getProp(regularString).c_str()
	    );
	all_pass = false;
    }

    printf("xmlNode: %s\n", doc.toString().c_str());

    /********************control char test***************/

    // control chars for xml:
    //int controlChars[] = {7,8,9,10,12,45,13,27,127};
    //int controlChars[] = {1, 2, 3};
    std::vector<int> controlChars;
    controlChars.push_back(1);
    controlChars.push_back(7);
    controlChars.push_back(9);
    controlChars.push_back(27);
    controlChars.push_back(127);

    //won't pass, ok for xml?
    //controlChars.push_back(128);
    //controlChars.push_back(150);
    //controlChars.push_back(151);

    controlCharString = "regular";
    foreach(int &i, controlChars)
    {
	controlCharString += (char) i;
    }

    controlCharString += "String";

    printf("length of 'regularString': %u\n",
	   (unsigned int) FString("regularString").length());

    printf("length of string with control chars: %u\n",
	   (unsigned int) controlCharString.length());


    printf("controlCharString: %s\n", controlCharString.c_str());

    XMLNode node(controlCharString, root);

    node.setProp(regularString, controlCharString);

    printf("xmlNode: %s\n", doc.toString().c_str());

    if (node.getProp(regularString) != regularString)
    {
	printf("FAIL: did not get stripped value back\n");
	printf("expected %s, got %s\n",
	       regularString.c_str(),
	       node.getProp(regularString).c_str()
	    );
	all_pass = false;
    }

    return (all_pass ? 0 : 1);
}

