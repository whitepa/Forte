#include "Forte.h"
#include "InternalRep.h"

int yyparse (void);

extern FILE *yyin;

using namespace DBC;

int main(int argc, char *argv[])
{
    int i = 0;
    FString outputPath, inputFile, customInclude, filenamePrefix, makefileName;
    while (!i)
    {
        int c;
        c = getopt(argc, argv, "i:o:c:p:m:");
        switch (c)
        {
        case -1:
            i = 1;
            break;
        case 'c':
            // custom include path
            customInclude = optarg;
            break;
        case 'o':
            // database layer output path
            outputPath = optarg;
            break;
        case 'i':
            // SQL input file
            inputFile = optarg;
            break;
        case 'p':
            filenamePrefix = optarg;
            break;
        case 'm':
            makefileName = optarg;
            break;
        }
    }
    if (inputFile.empty())
    {
        printf("An input file must be specified with '-i <filename>'\n");
        exit(1);
    }

    ParseContext pc;
    if (!customInclude.empty())
        pc.setIncludePath(customInclude);
    if (!outputPath.empty())
        pc.setOutputPath(outputPath);
    if (!filenamePrefix.empty())
        pc.setFilenamePrefix(filenamePrefix);
    if (!makefileName.empty())
        pc.setMakefileName(makefileName);
    // open the input file
    if ((yyin = fopen(inputFile, "r"))==NULL)
    {
        perror(inputFile.c_str());
        exit(1);
    }
    try
    {
        // parse the input
        if (!yyparse())
        {
            // perform validations
            pc.validate();
            // generate code and makefile include
            pc.generateCPP();
            pc.generateMk();
        }
        else
        {
            printf("PARSE FAILED\n");
            exit(1);
        }
    }
    catch (Exception &e)
    {
        printf("Exception thrown: %s\n", e.what());
        exit(1);
    }
}

