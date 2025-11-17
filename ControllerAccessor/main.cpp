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

#include <memory>

static int gRetryCount = 100;
static int gRetryDelayMs = 500;

using namespace WPEFramework;
using namespace WPEFramework::Core;
/**
 * @brief Display a help message for the tool
 */
static void displayUsage()
{
    printf("Usage: PluginActivator <option(s)> [callsign]\n");
    printf("    Utility that access the Controller and gets config n\n");
    printf("    -h, --help          Print this help and exit\n");
    printf("    -r, --retries       Maximum amount of retries to attempt to start the plugin before giving up\n");
    printf("    -d, --delay         Delay (in ms) between each attempt to start the plugin if it fails\n");
    printf("\n");
}

/**
 * @brief Parse the provided command line arguments
 *
 * Must be given the name of the plugin to activate, everything else
 * is optional and will fallback to sane defaults
 */
static void parseArgs(const int argc, char** argv)
{
    if (argc == 1) {
        displayUsage();
        exit(EXIT_SUCCESS);
    }

    struct option longopts[] = {
        { "help", no_argument, nullptr, (int)'h' },
        { "retries", required_argument, nullptr, (int)'r' },
        { "delay", required_argument, nullptr, (int)'d' },
        { nullptr, 0, nullptr, 0 }
    };

    opterr = 0;

    int option;
    int longindex;

    while ((option = getopt_long(argc, argv, "hr:d:", longopts, &longindex)) != -1) {
        switch (option) {
        case 'h':
            displayUsage();
            exit(EXIT_SUCCESS);
            break;
        case 'r':
            gRetryCount = std::atoi(optarg);
            if (gRetryCount < 0) {
                fprintf(stderr, "Error: Retry count must be > 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            gRetryDelayMs = std::atoi(optarg);
            if (gRetryDelayMs < 0) {
                fprintf(stderr, "Error: Delay ms must be > 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Warning: Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Warning: Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Warning: Unknown option character `\\x%x'.\n", optopt);

            exit(EXIT_FAILURE);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    }

    optind++;
    for (int i = optind; i < argc; i++) {
        printf("Warning: Non-option argument %s ignored\n", argv[i]);
    }
}

class ControllerAccessor {
public:
    ControllerAccessor()
       : _connector()
    {

    }
    ~ControllerAccessor() = default;

    bool access(const uint8_t maxRetries, const uint16_t retryDelayMs)
    {
        // Attempt to open the plugin shell
        bool success = false;
        int currentRetry = 1;

        while (!success && currentRetry <= maxRetries) {
            printf("ControllerAccesor: Attempting to access controller - attempt %d/%d", currentRetry, maxRetries);

            if (_connector.IsOperational() == false) {
                uint32_t result = _connector.Open(RPC::CommunicationTimeOut, ControllerConnector::Connector());
                if (result != Core::ERROR_NONE) {
                    printf("ControllerAccesor: Failed to get controller interface, error %u (%s)", result, Core::ErrorToString(result));
                }
            }

            PluginHost::IShell* controller = _connector.ControllerInterface();
            
            if(controller != nullptr){
                if((maxRetries - currentRetry) == 1)
                {
                    std::string cl = controller->ConfigLine();
                    controller->Substitute(cl);
                    printf("Called Substitute\n");
                }
                else             
                {
                    std::string cl = controller->ConfigLine();
                    printf("Got config\n");
                }
            }
            currentRetry++;

            _connector.Close(RPC::CommunicationTimeOut);

            // Sleep, then try again
            SleepMs(retryDelayMs);

        }

        return true;
    }



private:
    using ControllerConnector = RPC::SmartControllerInterfaceType<Exchange::Controller::ILifeTime>;

private:
    ControllerConnector _connector;
};


int main(int argc, char* argv[])
{
    parseArgs(argc, argv);
    bool success;

    {
        ControllerAccessor ca;
        success = ca.access(gRetryCount, gRetryDelayMs);
    }

    Core::Singleton::Dispose();
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}