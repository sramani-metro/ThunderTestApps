/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "Module.h"

#ifdef ENABLE_SECURITY_AGENT
#include <securityagent/securityagent.h>
#endif

#include "../JSONRPCPlugin/Data.h"

using namespace WPEFramework;

bool ParseOptions(int argc, char** argv, uint32_t& limit, uint32_t& delayMs, uint32_t& justDelay)
{
    int index = 1;
    limit = 10;
    delayMs = 300;
    bool showHelp = false;

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-l") == 0) {
            limit = atoi(argv[index + 1]);
            index++;
        } else if (strcmp(argv[index], "-d") == 0) {
            delayMs = atoi(argv[index + 1]);
            index++;
        }else if (strcmp(argv[index], "-j") == 0) {
            justDelay = atoi(argv[index + 1]);
            index++;
        } else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    return (showHelp);
}

int main(int argc, char** argv)
{
    uint32_t limit, delay, justDelay = 0;

    ParseOptions(argc, argv, limit, delay, justDelay);

    {
        printf("Preparing JSONRPC!!!\n");

        #ifdef __WINDOWS__
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:25555")));
        #else
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:55555")));
        #endif

        JSONRPC::LinkType<Core::JSON::IElement> remoteObject(_T("JSONRPCPlugin.1"), _T("client.events.1"));

        if(justDelay != 0)
        {
            Core::JSON::String result;
            remoteObject.Invoke<void, Core::JSON::String>(1000, _T("time"), result);
            printf("received time: %s\n", result.Value().c_str());
            SleepMs(justDelay);
        } else {
            for (int i = 0; i < limit; ++i)
            {
                Core::JSON::String result;
                remoteObject.Invoke<void, Core::JSON::String>(1000, _T("time"), result);
                printf("received time: %s\n", result.Value().c_str());
                SleepMs(delay);
            }
        }
    }

    printf("Leaving app.\n");

    Core::Singleton::Dispose();

    return (0);
}
