#include "JSONUtils.hpp"

#include "filereadstream.h"
#include "filewritestream.h"
#include "prettywriter.h"
#include "stringbuffer.h"

#include <maya/MGlobal.h>

Document loadJSON(string inputFile)
{
    Document d;
    FILE *fp = fopen(inputFile.c_str(), "r"); // non-Windows use "r"

    MGlobal::displayInfo(MString("Loading JSON file ") + inputFile.c_str());

    char readBuffer[65536];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    d.ParseStream(is);

    fclose(fp);

    return d;
}

void writeJSON(Document &d, string outputFile)
{
    FILE *fp = fopen(outputFile.c_str(), "w"); // non-Windows use "w"

    if (fp != NULL)
    {
        MGlobal::displayInfo(MString("Writing JSON file ") +
                             outputFile.c_str());

        char writeBuffer[65536];
        FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));

        PrettyWriter<FileWriteStream> writer(os);
        d.Accept(writer);

        fclose(fp);
    }
    else
    {
        MGlobal::displayError("Failed to get file pointer");
    }
}
