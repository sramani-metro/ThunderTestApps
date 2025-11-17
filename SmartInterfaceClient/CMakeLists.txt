# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10.3)

project(SmartInterfaceClient)

set(CMAKE_CXX_STANDARD 11)

find_package(WPEFramework)

find_package(${NAMESPACE}Core REQUIRED)
find_package(${NAMESPACE}COM REQUIRED)
find_package(${NAMESPACE}Plugins REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_executable(SmartInterfaceClient
    Module.cpp
    main.cpp
)

target_link_libraries(SmartInterfaceClient
    PRIVATE
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}COM::${NAMESPACE}COM
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    ${NAMESPACE}SimpleCOMRPCInterface::${NAMESPACE}SimpleCOMRPCInterface
    CompileSettingsDebug::CompileSettingsDebug
)

install(
    TARGETS SmartInterfaceClient
    RUNTIME DESTINATION bin
)
