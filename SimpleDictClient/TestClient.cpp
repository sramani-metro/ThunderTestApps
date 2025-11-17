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
 
#include <iostream>



#define MODULE_NAME TestClient

#include <core/core.h>
#include <com/com.h>
#include <definitions/definitions.h>
#include <plugins/Types.h>
#include <interfaces/IDictionary.h>
#include <iostream>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;


class Dictionary : public RPC::SmartInterfaceType<Exchange::IDictionary > {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IDictionary >;
public:
    Dictionary(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass() {
        uint32_t result = BaseClass::Open(waitTime, node, callsign);
	printf("SmartInterfaceType Open returned %u",result);
    }
    ~Dictionary() {
        BaseClass::Close(Core::infinite);
    }

public:
    bool Get(const string& nameSpace, const string& key, string& value ) const {
        bool result = false;
        const Exchange::IDictionary* impl = BaseClass::Interface();

        if (impl != nullptr) {
            if(impl->Get(nameSpace, key, value) == Core::ERROR_NONE)
            {
                result = true;
            }
            impl->Release();
        }

        return (result);
    }
    bool Set(const string& nameSpace, const string& key, const string& value) {
        bool result = false;
        Exchange::IDictionary* impl = BaseClass::Interface();
	
        if (impl != nullptr) {
            if(impl->Set(nameSpace, key, value) == Core::ERROR_NONE)
            {
                result = true;
            }
            impl->Release();
	        printf("Set returned %d\n",result);
        }
        else
        {
            printf("impl nullptr \n");
        }
        return (result);
    }

    void Trigger()
    {
       Exchange::IDictionary* impl = BaseClass::Interface();
                
        if (impl != nullptr) {
            printf("Release 1\n");
            impl->Release();
            printf("Release 2\n");
	    impl->Release();
        }   

    }


private:
    void Operational(const bool upAndRunning) {
        printf("Operational state of Dictionary: %s\n", upAndRunning ? _T("true") : _T("false"));
    }
};


int main(int /* argc */, char** /* argv */)
{
        std::cout << "TestClient" <<std::endl;


        Core::NodeId nodeId("/tmp/communicator");
       {
        std::cout << "Creating Dictionary SmartLink" <<std::endl;
        Dictionary  dictionary(3000, nodeId, _T("Dictionary"));
        char keyPress;
        uint32_t counter = 8;

        std::cout << "Looping to get options" <<std::endl;
        do {
            keyPress = toupper(getchar());
            
            switch (keyPress) {
            case 'O': {
                printf("Operations state issue: %s\n", dictionary.IsOperational() ? _T("true") : _T("false"));
                break;
            }
            case 'S': {

                string value = Core::NumberType<int32_t>(counter++).Text();
                if (dictionary.Set(_T("/name"), _T("key"), value) == true) {
                    printf("Set value: %s\n", value.c_str());
                }
                else
                {
                printf("Set failed\n");
                }
		
                break;
            }
            case 'G': {
                string value;
                if (dictionary.Get(_T("/name"), _T("key"), value) == true) {
                    printf("Get value: %s\n", value.c_str());
                }
                break;

            }
            case 'X': {
                uint32_t count = 0;
                while (count++ != 500000) {
                    string value = Core::NumberType<int32_t>(counter++).Text();
                    if (dictionary.Set(_T("/name"), _T("key"), value) == true) {
                        if (dictionary.Get(_T("/name"), _T("key"), value) == true) {
                            printf("Iteration %6i: Set/Get value: %s\n", count, value.c_str());
                        }
                    }
                }
                break;

            }
            case 'T': {
               std::cout << "Triggering" << std::endl;
               dictionary.Trigger();
               break;
            }
            case 'Q': break;
            default: break;
            };
        } while (keyPress != 'Q');
     }
    printf("Prior to the call Dispose\n");
    Core::Singleton::Dispose();
    printf("Completed the call Dispose\n");

    return 0;
}
