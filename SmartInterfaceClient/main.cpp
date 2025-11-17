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

/*
 * Example: Using SmartInterfaceType to build a Thunder client application
 *
 * This example demonstrates how to build a client application using Thunder's
 * SmartInterfaceType to interact with a COMRPC interface exposed by a ThunderPlugin.
 *
 * The example shows how to:
 *   - Instantiate and use SmartInterfaceType for connecting to a remote interface.
 *   - Register and unregister for asynchronous notifications (e.g., wallclock updates).
 *   - Safely handle locking and callback behavior to avoid common pitfalls like deadlocks.
 *
 * ⚠️ Notes:
 *   - Register()/Unregister() must not be wrapped with internal locks (like _apiLock),
 *     as they may perform COMRPC communication and invoke callbacks synchronously.
 *   - Callback methods should use locking cautiously and only if necessary to update
 *     shared state, avoiding reentrant deadlocks.
 *
 * Intended for developers integrating with ThunderPlugins using COMRPC from client apps.
 */

#include "Module.h"
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"
#include <chrono>
#include <ctime>

using namespace WPEFramework;
using namespace WPEFramework::Core;


class SmartInterfaceClient : public RPC::SmartInterfaceType<Exchange::IWallClock> {
    private:
        static constexpr const TCHAR* Callsign = _T("SimpleCOMRPCPluginServer");

        using BaseClass = RPC::SmartInterfaceType<Exchange::IWallClock>;

        class Sink : public Exchange::IWallClock::ICallback {
            public:
                Sink(const Sink&) = delete;
                Sink& operator= (const Sink&) = delete;
            
                Sink(SmartInterfaceClient& parent)
                    : _parent(parent) {
                };
                ~Sink() override {
                }
            
            public:
                BEGIN_INTERFACE_MAP(Sink)
                    INTERFACE_ENTRY(Exchange::IWallClock::ICallback)
                END_INTERFACE_MAP
            
                uint16_t Elapsed(const uint16_t seconds) override {
                    
                    return _parent.Elapsed(seconds);
                }
            
            private:
                SmartInterfaceClient& _parent;
            };


    private:
    SmartInterfaceClient(uint32_t secs)
            : BaseClass()
            , _wc(nullptr)
            , _sink(*this)
            , _seconds(secs)
        {
            uint32_t result = BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), Callsign);
            printf("SmartInterfaceClient Connection to %s status %u\n",Callsign, result);
        }

    ~SmartInterfaceClient()
    {
        BaseClass::Close(Core::infinite);
    }
    SmartInterfaceClient(const SmartInterfaceClient&) = delete;
    SmartInterfaceClient& operator=(const SmartInterfaceClient&) = delete;
    void Operational(const bool upAndRunning) override
    {
        _apiLock.Lock();

        if (upAndRunning) {
            printf("We are upAndRunning\n");
            if (_wc == nullptr) {
                _wc = BaseClass::Interface();
                if(_wc != nullptr && _seconds > 0)
                {   
                    printf("Arming\n");
                    Arm(_seconds);
                }
            }
        } else {
            printf("We are Down\n");
            if (_wc != nullptr) {
                printf("Disarming\n");
                Disarm();
                _wc->Release();
                _wc = nullptr;
            }
        }
        _apiLock.Unlock();
    }

    public:
        // ⚠️ Callback Locking Warning:
        // Avoid using _apiLock in this callback if it's also held during Register()/Unregister().
        // If the remote service triggers this callback *synchronously* during Register(),
        // and _apiLock is already held in the calling thread, it will result in a deadlock.
        // To prevent this, never hold _apiLock when calling Register()/Unregister(),
        // and ensure locking in callbacks is minimal, necessary, and well-documented.
        uint16_t Elapsed(const uint16_t seconds)
        {
            _apiLock.Lock();
            printf("The wallclock reports that %d seconds have elapsed since we where armed\n", seconds);
            _apiLock.Unlock();
            return _seconds;
        }

        // ⚠️ Important Note:
        // Avoid wrapping the below two calls which are primarily Register()/Unregister() calls
        // entirely with _apiLock or any internal locks. If you still have to use it, make sure
        // limit the scope of the lock to protect internal state of the object and release it
        // before calling COMRPC calls.
        // These functions may perform cross-process COMRPC communication, which can trigger
        // synchronous callbacks (e.g., Exchange::IWallClock::ICallback) into this component.
        // If such callbacks attempt to acquire _apiLock (which is already held), it can lead
        // to a deadlock. Always call Register()/Unregister() *before* acquiring locks.

        uint32_t Arm(const uint16_t seconds)
        {
            uint32_t errorCode = Core::ERROR_UNAVAILABLE;
            // _apiLock.Lock();

            uint32_t result = _wc->Arm(seconds, &_sink);
            if (result == Core::ERROR_NONE) {
                printf("We set the callback on the wallclock. We will be updated\n");
            }
            else {
                printf("Something went wrong: %d\n", result);
            }
            // _apiLock.Unlock();
            return errorCode;
        }
        uint32_t Disarm()
        {
            // _apiLock.Lock();
            uint32_t result = _wc->Disarm(&_sink);

            printf("Disarm returned %d\n", result);
            if (result == Core::ERROR_NONE) {
                printf("We removed the callback from the wallclock. We will no longer be updated\n");
            }
            else if (result == Core::ERROR_NOT_EXIST) {
                printf("Looks like it was not Armed, or it fired already!\n");
            }
            else {
                printf("Something went wrong: %d\n", result);
            }
            // _apiLock.Unlock();
            return result;
        }
        uint64_t Now()
        {
            uint64_t now = 0;
            _apiLock.Lock();
            if (_wc != nullptr) {
                now = _wc->Now();
            }
            _apiLock.Unlock();
            return now;
        }

        static void Init (uint32_t secs)
        {
            _apiLock.Lock();
            if(_instance == nullptr)
            {
                _instance = new SmartInterfaceClient(secs);
            }
            _apiLock.Unlock();
        }

        static void Term ()
        {
            printf("Term Called from %d\n",::gettid());
            _apiLock.Lock();
            if(_instance != nullptr)
            {
                delete _instance;
                _instance = nullptr;
            }
            _apiLock.Unlock();

        }

        static SmartInterfaceClient& Instance()
        {
            return *_instance;
        }
    
    private:
        static SmartInterfaceClient* _instance;
        static Core::CriticalSection _apiLock;
        Exchange::IWallClock* _wc;
        Core::Sink<Sink> _sink;
        uint32_t _seconds;

}; // class SmartInterfaceClient


/*static */SmartInterfaceClient* SmartInterfaceClient::_instance = nullptr; 
/*static */Core::CriticalSection SmartInterfaceClient::_apiLock;


// C Wrappers

extern "C" {

void SmartInterfaceClient_Init(uint32_t secs)
{
    SmartInterfaceClient::Init(secs);
}

void SmartInterfaceClient_Term()
{
    SmartInterfaceClient::Term();
}

bool SmartInterfaceClient_IsOperational()
{
    return SmartInterfaceClient::Instance().IsOperational();
}

void SmartInterfaceClient_Arm(uint32_t secs)
{
    SmartInterfaceClient::Instance().Arm(secs);
}

void SmartInterfaceClient_Disarm()
{
    SmartInterfaceClient::Instance().Disarm();
}

uint64_t SmartInterfaceClient_Now()
{
    return SmartInterfaceClient::Instance().Now();
}

} //extern "C" 

int main(int argc, char* argv[])
{
    bool success;
    int element;
    {
        printf("SmartInterfaceClient Starting \n");
        SmartInterfaceClient_Init(1);

        uint64_t now = SmartInterfaceClient_Now();
        printf("SmartInterfaceClient Now - %ld\n", now);

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
                case 'E': exit(0); break;
                case 'Q': break;
            }
        }while (element != 'Q');

        now = SmartInterfaceClient_Now();
        printf("SmartInterfaceClient Now - %ld\n", now);

        SmartInterfaceClient_Term();
    }

    Core::Singleton::Dispose();
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}