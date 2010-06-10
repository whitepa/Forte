#include "Forte.h"

using namespace Forte;

ServiceConfig::ServiceConfig() {
    mConfig.clear();
}
ServiceConfig::ServiceConfig(const char *configFile) {
    mConfig.clear();
    ReadConfigFile(configFile);
}

void ServiceConfig::ReadConfigFile(const char *configFile) {
    // load the file
    bool loggedWarning = false;
    FString conf;
    FString::LoadFile(configFile, 0x100000, conf);

    FString line, name, val;
    FString::size_type pos;

    // parse file
    while (!conf.empty())
    {
        // get line
        pos = conf.find('\n');

        if (pos == NOPOS)
        {
            line = conf;
            conf.clear();
        }
        else
        {
            line = conf.Left(pos);
            conf = conf.Mid(pos + 1);
        }
        line.TrimLeft();
        line.TrimRight();
        if (line.empty() || line[0] == '#') continue;

        // parse line
        if ((pos = line.find('=')) != NOPOS)
        {
            // FORMAT: name = value
            name = line.Left(pos);
            name.TrimRight();
            name.MakeLower();

            val = line.Mid(pos + 1);
            val.TrimLeft();
        }
        else if ((pos = line.find('"')) != NOPOS)
        {
            // FORMAT: name "value"
            name = line.Left(pos);
            name.TrimRight();
            name.MakeLower();

            val = line.Mid(pos + 1);
            val.TrimRight("\"");
        }
	else
	{
	    if (!loggedWarning)
	    {
		loggedWarning = true;
		hlog(HLOG_WARN, "Invalid data found in config file: %s",
		     configFile);
	    }
            continue;
	}
        // interpret value?
        if (!val.empty() && val[0] == '$')
        {
            char *env = getenv(val.Mid(1).c_str());
            if (env != NULL) val = env;
        }

        // set value
        Set(name, val);
    }
//    Display();
}

void ServiceConfig::Display(void)
{
    StringHashMap::iterator i;
    cout << "Hash contents:" << endl;
    for (i = mConfig.begin(); i!=mConfig.end(); i++)
    {
        cout << (*i).first << "=" << (*i).second << endl;
    }
}
void ServiceConfig::Clear()
{
    AutoUnlockMutex lock(mMutex);
    mConfig.clear();
}

void ServiceConfig::Set(const char *key, const char *value)
{
    AutoUnlockMutex lock(mMutex);
    mConfig[key] = value;
}

FString ServiceConfig::Get(const char *key)
{
    AutoUnlockMutex lock(mMutex);
    return mConfig[key];
}

int ServiceConfig::GetInteger(const char *key)
{
    return strtol(Get(key),NULL,10);
}

