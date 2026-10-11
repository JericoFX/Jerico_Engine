// Copyright (c) Contributors to the Open 3D Engine Project.
// SPDX-License-Identifier: Apache-2.0 OR MIT

#include <expat.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

#ifndef XML_STATIC
#error The engine consumes static Expat with XML_STATIC.
#endif
static_assert(XML_MAJOR_VERSION == 2 && XML_MINOR_VERSION == 4 && XML_MICRO_VERSION == 2);
static_assert(std::is_same_v<XML_Char, char> && std::is_same_v<XML_Index, long>);
static_assert(sizeof(void*) == 8 && sizeof(XML_Size) == 4);

namespace JE::LocalPackages::Expat
{
    std::unordered_set<void*> Allocations;
    size_t AllocationCalls = 0;
    bool OwnershipError = false;

    void* XMLCALL Allocate(size_t size)
    {
        void* pointer = std::malloc(size);
        if (pointer)
        {
            Allocations.insert(pointer);
            ++AllocationCalls;
        }
        return pointer;
    }

    void* XMLCALL Reallocate(void* pointer, size_t size)
    {
        if (!pointer)
        {
            return Allocate(size);
        }
        if (Allocations.erase(pointer) != 1)
        {
            OwnershipError = true;
            return nullptr;
        }
        void* replacement = std::realloc(pointer, size);
        if (replacement)
        {
            Allocations.insert(replacement);
        }
        else if (size != 0)
        {
            Allocations.insert(pointer);
        }
        return replacement;
    }

    void XMLCALL Free(void* pointer)
    {
        if (pointer)
        {
            if (Allocations.erase(pointer) != 1)
            {
                OwnershipError = true;
                return;
            }
            std::free(pointer);
        }
    }

    struct Document
    {
        XML_Parser parser{};
        std::vector<std::string> starts;
        std::vector<std::string> ends;
        std::vector<std::string> attributes;
        std::string text;
        XML_Size firstLine{};
        int externalCalls{};
    };

    void XMLCALL Start(void* data, const XML_Char* name, const XML_Char** attributes)
    {
        auto& document = *static_cast<Document*>(data);
        if (document.starts.empty())
        {
            document.firstLine = XML_GetCurrentLineNumber(document.parser);
        }
        document.starts.emplace_back(name);
        for (size_t index = 0; attributes[index]; index += 2)
        {
            document.attributes.emplace_back(attributes[index]);
            document.attributes.emplace_back(attributes[index + 1]);
        }
    }

    void XMLCALL End(void* data, const XML_Char* name)
    {
        static_cast<Document*>(data)->ends.emplace_back(name);
    }

    void XMLCALL Text(void* data, const XML_Char* text, int length)
    {
        static_cast<Document*>(data)->text.append(text, static_cast<size_t>(length));
    }

    int XMLCALL RejectExternal(XML_Parser parser, const XML_Char*, const XML_Char*, const XML_Char*, const XML_Char*)
    {
        ++static_cast<Document*>(XML_GetUserData(parser))->externalCalls;
        return XML_STATUS_ERROR; // Never open a file or URL supplied by XML.
    }

    void SetHandlers(Document& document)
    {
        XML_SetUserData(document.parser, &document);
        XML_SetElementHandler(document.parser, Start, End);
        XML_SetCharacterDataHandler(document.parser, Text);
        XML_SetExternalEntityRefHandler(document.parser, RejectExternal);
    }

    bool Baseline()
    {
        const auto version = XML_ExpatVersionInfo();
        bool dtd = false;
        bool namespaces = false;
        bool context = false;
        for (const XML_Feature* feature = XML_GetFeatureList(); feature->feature != XML_FEATURE_END; ++feature)
        {
            dtd = dtd || feature->feature == XML_FEATURE_DTD;
            namespaces = namespaces || feature->feature == XML_FEATURE_NS;
            context = context || (feature->feature == XML_FEATURE_CONTEXT_BYTES && feature->value == 1024);
            if (feature->feature == XML_FEATURE_UNICODE || feature->feature == XML_FEATURE_LARGE_SIZE)
            {
                return false;
            }
        }
        return version.major == 2 && version.minor == 4 && version.micro == 2
            && std::strcmp(XML_ExpatVersion(), "expat_2.4.2") == 0 && dtd && namespaces && context;
    }

    bool Streaming(Document& document)
    {
        const std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE room [<!ENTITY label 'door'>]>\n"
            "<room id=\"caf\xC3\xA9\"><wall>&label; &amp; &#x1F642;</wall></room>";
        if (XML_SetEncoding(document.parser, "utf-8") != XML_STATUS_OK)
        {
            return false;
        }
        for (size_t index = 0; index < xml.size(); ++index)
        {
            if (XML_Parse(document.parser, xml.data() + index, 1, index + 1 == xml.size()) != XML_STATUS_OK)
            {
                return false;
            }
        }
        return document.starts == std::vector<std::string>{ "room", "wall" }
            && document.ends == std::vector<std::string>{ "wall", "room" }
            && document.attributes == std::vector<std::string>{ "id", "caf\xC3\xA9" }
            && document.text == "door & \xF0\x9F\x99\x82" && document.firstLine == 3;
    }

    bool Namespaces(Document& document)
    {
        const char* xml = "<room xmlns=\"urn:je\" xmlns:o=\"urn:other\" o:id=\"7\"><o:wall/></room>";
        return XML_Parse(document.parser, xml, static_cast<int>(std::strlen(xml)), 1) == XML_STATUS_OK
            && document.starts == std::vector<std::string>{ "urn:je|room", "urn:other|wall" }
            && document.ends == std::vector<std::string>{ "urn:other|wall", "urn:je|room" }
            && document.attributes == std::vector<std::string>{ "urn:other|id", "7" };
    }

    bool Errors(Document& document)
    {
        struct Invalid { const char* xml; XML_Error error; };
        const Invalid cases[] = {
            { "<room>\n<wall></room>", XML_ERROR_TAG_MISMATCH },
            { "<room>", XML_ERROR_NO_ELEMENTS },
            { "<room>\xC3\x28</room>", XML_ERROR_INVALID_TOKEN },
            { "<!DOCTYPE room [<!ENTITY ext SYSTEM 'file:///JE-never-read.xml'>]><room>&ext;</room>",
                XML_ERROR_EXTERNAL_ENTITY_HANDLING }
        };
        for (const auto& invalid : cases)
        {
            if (!XML_ParserReset(document.parser, "utf-8"))
            {
                return false;
            }
            document.starts.clear();
            document.ends.clear();
            document.text.clear();
            SetHandlers(document);
            if (XML_Parse(document.parser, invalid.xml, static_cast<int>(std::strlen(invalid.xml)), 1) != XML_STATUS_ERROR
                || XML_GetErrorCode(document.parser) != invalid.error
                || !XML_ErrorString(XML_GetErrorCode(document.parser)) || XML_GetCurrentLineNumber(document.parser) < 1)
            {
                return false;
            }
        }
        if (document.externalCalls != 1 || !XML_ParserReset(document.parser, "utf-8"))
        {
            return false;
        }
        document.starts.clear();
        document.ends.clear();
        document.text.clear();
        SetHandlers(document);
        const char* valid = "<recovered>valid</recovered>";
        return XML_Parse(document.parser, valid, static_cast<int>(std::strlen(valid)), 1) == XML_STATUS_OK
            && XML_GetErrorCode(document.parser) == XML_ERROR_NONE
            && document.starts == std::vector<std::string>{ "recovered" } && document.text == "valid";
    }

    bool Memory(Document& document)
    {
        void* allocation = XML_MemMalloc(document.parser, 32);
        if (!allocation)
        {
            return false;
        }
        std::memset(allocation, 0x5A, 32);
        void* grown = XML_MemRealloc(document.parser, allocation, 4096);
        if (!grown)
        {
            XML_MemFree(document.parser, allocation);
            return false;
        }
        const bool retained = static_cast<unsigned char*>(grown)[31] == 0x5A;
        XML_MemFree(document.parser, grown);
        const std::string xml = "<room>" + std::string(100000, 'x') + "</room>";
        void* buffer = XML_GetBuffer(document.parser, static_cast<int>(xml.size()));
        if (!buffer)
        {
            return false;
        }
        std::memcpy(buffer, xml.data(), xml.size());
        return retained && XML_ParseBuffer(document.parser, static_cast<int>(xml.size()), 1) == XML_STATUS_OK
            && document.starts == std::vector<std::string>{ "room" } && document.text == std::string(100000, 'x');
    }
}

int main(int argc, char** argv)
{
    using namespace JE::LocalPackages::Expat;
    if (argc != 2 || !Baseline())
    {
        std::cerr << "Expat consumer requires one case and exact 2.4.2 char/DTD/NS/context ABI\n";
        return 1;
    }
    const std::string test = argv[1];
    const XML_Memory_Handling_Suite suite{ Allocate, Reallocate, Free };
    const char separator = '|';
    Document document;
    document.parser = XML_ParserCreate_MM(nullptr, &suite, test == "namespaces" ? &separator : nullptr);
    if (!document.parser)
    {
        return 1;
    }
    SetHandlers(document);
    const bool passed = test == "streaming" ? Streaming(document)
        : test == "namespaces" ? Namespaces(document)
        : test == "errors" ? Errors(document)
        : test == "memory" && Memory(document);
    XML_ParserFree(document.parser);
    const bool owned = AllocationCalls > 0 && Allocations.empty() && !OwnershipError;
    std::cout << "CASE " << test << ": " << (passed && owned ? "PASS" : "FAIL") << '\n';
    return passed && owned ? 0 : 1;
}
