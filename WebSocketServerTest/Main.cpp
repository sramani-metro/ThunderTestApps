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
#include <core/core.h>
#include <websocket/websocket.h>
#include <condition_variable>
#include <mutex>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Tests {

    class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector("0.0.0.0:55555")
            {
                Add(_T("connector"), &Connector);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
        };


    class Message : public Core::JSON::Container {
    public:
        Message(const Message&) = delete;
	    Message& operator= (const Message&) = delete;

	    Message()
            : Core::JSON::Container()
            , EventType()
            , Event()
        {
            Add(_T("eventType"), &EventType);
            Add(_T("event"), &Event);
        }

        ~Message()
        {
        }

    public:
	    Core::JSON::String EventType;
	    Core::JSON::String Event;
    };

    class Factory : public Core::ProxyPoolType<Message> {
    public:
	    Factory() = delete;
	    Factory(const Factory&) = delete;
	    Factory& operator= (const Factory&) = delete;

	    Factory(const uint32_t number) : Core::ProxyPoolType<Message>(number)
        {
	    }

	    virtual ~Factory()
        {
	    }

    public:
	    Core::ProxyType<Core::JSON::IElement> Element(const string&)
        {
		    return (Core::ProxyType<Core::JSON::IElement>(Core::ProxyPoolType<Message>::Element()));
	    }
    };

    template<typename INTERFACE>
    class JsonSocketServer : public Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> {
    private:
	    typedef Core::StreamJSONType< Web::WebSocketServerType<Core::SocketStream>, Factory&, INTERFACE> BaseClass;

    public:
        JsonSocketServer() = delete;
        JsonSocketServer(const JsonSocketServer&) = delete;
	    JsonSocketServer& operator=(const JsonSocketServer&) = delete;

        JsonSocketServer(const SOCKET& socket, const Core::NodeId& remoteNode, Core::SocketServerType<JsonSocketServer<INTERFACE>>*)
            : BaseClass(2, _objectFactory, false, false, false, socket, remoteNode, 512, 512)
		    , _objectFactory(1)
        {
	    }

        virtual ~JsonSocketServer()
        {
        }

    public:
	    virtual bool IsIdle() const
        {
            return (true);
        }

	    virtual void StateChange()
        {
		    if (this->IsOpen()) {
                std::unique_lock<std::mutex> lk(_mutex);
                _done = true;
                _cv.notify_one();
            }
        }

        bool IsAttached() const
        {
            return (this->IsOpen());
        }

        virtual void Received(Core::ProxyType<Core::JSON::IElement>& jsonObject)
        {
            this->Submit(jsonObject);
        }

        virtual void Send(Core::ProxyType<Core::JSON::IElement>& jsonObject)
        {
	    }

        static bool GetState()
        {
            return _done;
        }

    private:
	    Factory _objectFactory;
        static bool _done;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    template<typename INTERFACE>
    std::mutex JsonSocketServer<INTERFACE>::_mutex;
    template<typename INTERFACE>
    std::condition_variable JsonSocketServer<INTERFACE>::_cv;
    template<typename INTERFACE>
    bool JsonSocketServer<INTERFACE>::_done = false;

} // Tests
} // WPEFramework

using namespace WPEFramework;
using namespace WPEFramework::Tests;

int main (int argc, char* argv[])
{

     printf("jsonWebSocketServer - Init\n");
     Config config;
     Core::NodeId source(config.Connector.Value().c_str());

     {
	     Core::SocketServerType<JsonSocketServer<Core::JSON::IElement>> jsonWebSocketServer(Core::NodeId(source, source.PortNumber()));
	     jsonWebSocketServer.Open(Core::infinite);
	 
	     printf("jsonWebSocketServer listnening\n");

	     std::unique_lock<std::mutex> lk(JsonSocketServer<Core::JSON::IElement>::_mutex);
	     while (!JsonSocketServer<Core::JSON::IElement>::GetState()) {
			JsonSocketServer<Core::JSON::IElement>::_cv.wait(lk);
	     }

	     printf("jsonWebSocketServer Client Connected\n");

	     int element;

		do {
		    printf("\n>");
		    element = toupper(getchar());

		    switch (element) {
		    case 'Q': break;
		    default: break;
		    }

		} while (element != 'Q');
            jsonWebSocketServer.Close(1000);
     }
     Core::Singleton::Dispose();
     return 0;
}

