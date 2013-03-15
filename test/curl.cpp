#include "Forte.h"
#include "Curl.h"
#include "ansi_test.h"

using namespace Forte;

// not a unit test. i just need to find out if this will work

typedef struct {
    FString buf;
    int errorCode; // 0 = no error, non zero = REPL_ERROR_CODE
} cb_data_t;   // callback data for applying diff packages

size_t cbControlFile(void *buffer, size_t size, size_t nmemb,
                     void *userp)
{
    //cb_data_t *data = reinterpret_cast<cb_data_t*>(userp);
    printf("Got control file data: %s\n", (char*) buffer);
    return size * nmemb;
}

size_t cbHeader(void *buffer, size_t size, size_t nmemb,
                void *userp)
{
    //cb_data_t *data = reinterpret_cast<cb_data_t*>(userp);
    printf("Got header data: %s\n", (char*) buffer);
    return size * nmemb;
}

void testScaleReplDL()
{
    FString url = "http://qa:5161/repl"
        "/data/nph-mock-source.cgi?cf=case5000_ignore=1265017750/5000";

    Curl curl;
    unsigned int timeout;
    cb_data_t data; // we only need a the target and errorCode params

    curl.SetURL(url);
    curl.SetRecvCB(&cbControlFile, reinterpret_cast<void*>(&data));
    curl.SetRecvHeaderCB(&cbHeader, reinterpret_cast<void*>(&data));
    curl.SetNoSignal(true);   // no signals on timeouts
    timeout = 10;
    curl.SetConnectTimeout(timeout);
    curl.SetLowSpeed(10, timeout);    // < 10 bps threshold to set timeout
    curl.Perform();

}

int main(int argc, char *argv[])
{
    bool all_pass = true;
    ProcRunner pr;
    FileSystem fs;
    CurlInitializer ci;

    LogManager logManager;
    logManager.BeginLogging();
    logManager.SetGlobalLogMask(HLOG_ALL);

    try
    {
        Curl curl;
        curl.SetURL("notthere");
        curl.Perform();
        hlog(HLOG_ERR, "no error on notthere");
        throw Exception("no error on notthere");
    }
    catch (Curl::CurlException& e)
    {
        hlog(HLOG_INFO, "got nothere expected exception");
    }


    try
    {
        Curl curl;
        curl.SetURL("http://localhost/notthere");
        curl.Perform();
        hlog(HLOG_ERR, "no error on http://localhost/notthere");
        throw Exception("no error on http://localhost/notthere");
    }
    catch (Curl::CurlException& e)
    {
        hlog(HLOG_INFO, "got http://localhost/notthere expected exception");
    }

    FString dlInternalBuf;
    FString downloadURL = "http://localhost/~hardtnf/testfile1";
    {
        // not a unit test. i just need to find out if this will work
        Curl curl;
        curl.SetURL(downloadURL);
        curl.SetInternalCB();
        curl.Perform();
        dlInternalBuf = curl.mBuf;
        printf("%s", curl.mBuf.Left(50).c_str());
        hlog(HLOG_INFO, "did basic download");
    }

    {
        hlog(HLOG_INFO, "attempting to dl to file");
        FILE* outputFilePointer;
        pr.Run("/bin/rm ./dlfile", "", NULL, 120);
        outputFilePointer = fopen("./dlfile", "wx");

        if (outputFilePointer == NULL)
        {
            hlog(HLOG_ERR, "could not create outputFile");
            throw Exception("");
        }

        Curl curl;
        curl.SetOutputFile(outputFilePointer);
        curl.SetFollowRedirects(true);
        curl.SetURL(downloadURL);
        curl.Perform();

        fclose(outputFilePointer);

        if (!fs.FileExists("./dlfile"))
        {
            hlog(HLOG_ERR, "there is no dlfile");
            throw Exception("");
        }

        FString dlFile;
        dlFile = FString::LoadFile("./dlfile", 10240, dlFile);

        if (dlFile != dlInternalBuf)
        {
            hlog(HLOG_ERR, "dlFile != to dlInternalBuf");
            hlog(HLOG_INFO, "dlFile: %s\n dlInternalBuf: %s\n",
                 dlFile.c_str(),
                 dlInternalBuf.c_str());
            throw Exception("");
        }
        //pr.run("/bin/rm ./dlfile");
        hlog(HLOG_INFO, "done with dl to file");
    }

    try
    {
        testScaleReplDL();
        hlog(HLOG_ERR, "did not timeout in testScaleReplDl");
        throw Exception("did not timeout in testScaleReplDl");
    }
    catch (Curl::CurlException& e)
    {
        hlog(HLOG_INFO, "got expected timeout error in testScaleReplDl");
    }

    // done
    return (all_pass ? 0 : 1);
}

