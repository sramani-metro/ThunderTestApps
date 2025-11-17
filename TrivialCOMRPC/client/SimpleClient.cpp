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

#define MODULE_NAME SimpleClient

#include <core/core.h>
#include <com/com.h>
#include "../interface/ISimpleInterface.h"
#include <iostream>
#include <plugins/plugins.h>
#include <vector>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;
using namespace std;

enum class ServerType{
    PLUGIN_SERVER,
    STANDALONE_SERVER
};

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel, ServerType& type, string& callsign)
{
    int index = 1;
    bool showHelp = false;
    comChannel = Core::NodeId(Exchange::SimpleTestAddress);

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-connect") == 0) {
            comChannel = Core::NodeId(argv[index + 1]);
            type = ServerType::STANDALONE_SERVER;
            index++;
        }
        else if (strcmp(argv[index], "-plugin") == 0) {
#ifdef __WINDOWS__
            comChannel = Core::NodeId("127.0.0.1:62000");
#else
            comChannel = Core::NodeId("/tmp/communicator");
#endif
            type = ServerType::PLUGIN_SERVER;
            if ((index + 1) < argc) {
                callsign = string(argv[index + 1]);
            }
            else {
                callsign = _T("SimpleCOMRPCPluginServer");
            }
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
    ServerType type;
    string callsign;
    Exchange::IMath* math = nullptr;

    printf("\nSimpleClient is the counterpart for the SimpleServer\n");

    if (ParseOptions(argc, argv, comChannel, type, callsign) == true) {
        printf("Options:\n");
        printf("-connect <IP/FQDN>:<port> [default: %s]\n", Exchange::SimpleTestAddress);
        printf("-plugin <callsign> [use plugin server and not the stand-alone version]\n");
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(comChannel));

#if 0
        //Prerequisite - 
        //   1. Binder Service Manager running.
        //   2. EchoService Started and registered with SM

        Core::ProxyType<RPC::BinderClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create());

        // Binder Open
        client->Open();

        //Service Manager look up of EchoService
        math = client->Acquire<Exchange::IMath>(3000, "EchoService");


        uint16_t res;
        // binder_call
        math->Add(5, 6, res);


        //binder_close
        client->Close();


#endif



        printf("Channel: %s:[%d]\n\n", comChannel.HostAddress().c_str(), comChannel.PortNumber());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'M':
                if (math != nullptr) {
                    printf("There is no need to create the iface, we already have one!\n");
                } else {
                    if (client->IsOpen() == false) {
                        client->Open(2000);
                    }

                    if (client->IsOpen() == false) {
                        printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                        break;
                    } else {
                        if (type == ServerType::STANDALONE_SERVER) {
                            printf("Acquiring\n");
                            math = client->Acquire<Exchange::IMath>(8000, _T("Math"), ~0);
                        }
                        else {
                            Thunder::PluginHost::IShell* controller = client->Acquire<Thunder::PluginHost::IShell>(10000, _T("Controller"), ~0);
                            if (controller == nullptr) {
                                printf("Could not get the IShell* interface from the controller to execute the QueryInterfaceByCallsign!\n");
                            }
                            else {
                                math = controller->QueryInterfaceByCallsign<Exchange::IMath>(callsign);
                                controller->Release();
                            }
                        }
                    }

                    if (math == nullptr) {
                        client->Close(Core::infinite);
                        if (type == ServerType::STANDALONE_SERVER) {
                            printf("Tried aquiring the IMath, but it is not available\n");
                        }
                        else {
                            printf("Tried aquiring the IMath, but the plugin (%s) is not available\n", callsign.c_str());

                        }
                    } else {
                        printf("Acquired the IMath, ready for use\n");
                    }
                }
                break;
            case 'D':
                if (math == nullptr) {
                    printf("We can not destroy the math iface, because we have no math iface :-)\n");
                }
                else {
                    math->Release();
                    math = nullptr;
                    printf("Released the IMath, no more service available\n");
                }
                break;
            case 'A':
                if (math == nullptr) {
                    printf("We do not have a math interface, so we can not perform add\n");
                }
                else {
                    uint16_t x, y;
                    uint16_t sum;
                    cout << "Type a number: ";
                    cin >> x;
                    cout << "Type another number: ";
                    cin >> y;
                    math->Add(x,y,sum);
                    printf("The Sum of %d and %d is : %d\n", x, y, sum);
                }
                break;

            case 'S':
                if (math == nullptr) {
                    printf("We do not have a math interface, so we can not perform Sub\n");
                }
                else {
                    uint16_t x, y;
                    uint16_t diff;
                    cout << "Type a number: ";
                    cin >> x;
                    cout << "Type another number: ";
                    cin >> y;
                    math->Sub(x,y,diff);
                    printf("The Difference of %d and %d is: %d\n", x,y,diff);
                }
                break;
	    case 'E': exit(0); break;
            case 'Q': break;
            case'?':
                printf("Options available:\n");
                printf("=======================================================================\n");
                printf("<M> Create the Math interface, usefull for adding or subtracting numbers.\n");
                printf("<D> Destroy the Math interface.\n");
                printf("<A> Add 2 numbers.\n");
                printf("<S> Subtract 2 numbers.\n");
                printf("<Q> We are done playing around, eave the application properly.\n");
                printf("<E> Eject, this is an emergency, bail out, just kill the app.\n");
                printf("<?> Have no clue what I can do, tell me.\n");
            default: break;
            }

        } while (element != 'Q');

        if(math != nullptr)
        {
            math->Release();
        }

        if (client->IsOpen() == true) {
            client->Close(Core::infinite);
        }
    }

    Core::Singleton::Dispose();

    return 0;
}
