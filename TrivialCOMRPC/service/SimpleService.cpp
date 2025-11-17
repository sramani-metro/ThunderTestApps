/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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

#define MODULE_NAME SimpleService

#include <core/core.h>
#include <com/com.h>
#include "../interface/ISimpleInterface.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE);

using namespace Thunder;

class COMServer : public RPC::Communicator {
private:
    class Math : public Exchange::IMath {
    public:
        Math(const Math&) = delete;
        Math& operator= (const Math&) = delete;

        Math() {
        }
        ~Math() override {
        }

    public:
        // Inherited via IMath
        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = A + B;
            return (Core::ERROR_NONE);
        }
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = A - B;
            return (Core::ERROR_NONE);
        }

        BEGIN_INTERFACE_MAP(Math)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP
    };

public:
    COMServer() = delete;
    COMServer(const COMServer&) = delete;
    COMServer& operator=(const COMServer&) = delete;

    COMServer(
        const Core::NodeId& source,
        const string& proxyServerPath,
        const Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >& engine)
        : RPC::Communicator(
            source, 
            proxyServerPath, 
            Core::ProxyType<Core::IIPCServer>(engine))
    {
        // Once the socket is opened the first exchange between client and server is an 
        // announce message. This announce message hold information the otherside requires
        // like, where can I find the ProxyStubs that I need to load, what Trace categories
        // need to be enabled.
        // Extensibility allows to be "in the middle" of these messages and to chose on 
        // which thread this message should be executes. Since the message is coming in 
        // over socket, the announce message could be handled on the communication thread
        // or better, if possible, it can be run on the thread of the engine we have just 
        // created.
        Open(Core::infinite);
    }
    ~COMServer() override
    {
        Close(Core::infinite);
    }

private:
    void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) override
    {
        void* result = nullptr;
        printf("Acquire:");

        // Currently we only support version 1 of the IRPCLink :-)
        if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {
            
            if (interfaceId == ::Exchange::IMath::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::ServiceType<Math>::Create<Exchange::IMath>();
            }
        }
        return (result);
    }
    void Offer(Core::IUnknown* remote, const uint32_t interfaceId) override
    {
    }
    void Revoke(const Core::IUnknown* remote, const uint32_t interfaceId) override
    {
    }
private:
    Exchange::IMath* _remoteEntry;
};

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel, string& psPath)
{
    int index = 1;
    bool showHelp = false;
    comChannel = Core::NodeId(Exchange::SimpleTestAddress);
    psPath = _T("./PS");

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-listen") == 0) {
            comChannel = Core::NodeId(argv[index + 1]);
            index++;
        }
        else if (strcmp(argv[index], "-path") == 0) {
            psPath = argv[index + 1];
            index++;
        }
        else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    return (showHelp);
}


int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    Core::NodeId comChannel;
    string psPath;

    printf("\nSimple COMRPC Service offering IMath interface\n");

    if (ParseOptions(argc, argv, comChannel, psPath) == true) {
        printf("Options:\n");
        printf("-listen <IP/FQDN>:<port> [default: %s]\n", Exchange::SimpleTestAddress);
        printf("-path <Path to the location of the ProxyStubs> [default: ./PS]\n");
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        COMServer server(comChannel, psPath, Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >::Create());
        printf("Channel:        %s:[%d]\n", comChannel.HostAddress().c_str(), comChannel.PortNumber());
        printf("ProxyStub path: %s\n\n", psPath.c_str());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'Q': break;
            default: break;
            }

        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return 0;
}
