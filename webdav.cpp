// webdav.cpp : Definiert die exportierten Funktionen für die DLL-Anwendung.
//
// https://www.windowspage.de/tipps/022703.html
// https://help.dreamhost.com/hc/en-us/articles/216473357-Accessing-WebDAV-with-Windows
// https://support.microsoft.com/de-de/topic/update-zum-aktivieren-von-tls-1-1-und-tls-1-2-als-sichere-standardprotokolle-in-winhttp-in-windows-c4bd73d2-31d7-761e-0178-11268bb10392

#define _HAS_STD_BYTE 0
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <iostream>
#include <map>
#include <deque>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <locale>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <fstream>
#include <functional>
#include <regex>
#include <thread>
#include <fcntl.h>

#if defined(_WIN32) || defined(_WIN64)
// #define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
// #include <experimental/filesystem>
#include <filesystem>
//namespace fs = std::experimental::filesystem;
namespace fs = std::filesystem;
#else
#include <filesystem>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include "ConfFile.h"
#include "SrvLib/Service.h"
#include "tinyxml2/tinyxml2.h"
#include "FastCgi/FastCgi.h"
#include "md5/md5.h"
#include "Base64.h"

std::wstring mbs2ws(const std::string src);
std::string ws2mbs(const std::wstring src);
std::wstring utf82ws(const std::string src);
std::string ws2utf8(const std::wstring src);

#if defined(_WIN32) || defined(_WIN64)
//#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#include <Windows.h>
#include <sys/utime.h>
#include <conio.h>
#include <io.h>
#define ConvertToByte(x) ws2utf8(x)
#define FN_STR(x) x
#define NATIV_STR(x) x.wstring()
#define WsFromNativ(x) x
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#else
//#include <sys/types.h>
#include <termios.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#define _stat64 stat64
#define _wstat64 stat64
#define _stat stat
#define _utimbuf utimbuf
#define _utime utime
#define _mkgmtime timegm
#define _wremove remove
#define _wopen open
#define _close close
#define _write write
#define _read read
#define _lseek lseek
#define _S_IFDIR S_IFDIR
#define _S_IFREG S_IFREG
#define ConvertToByte(x) x
#define FN_STR(x) ws2utf8(x)
#define NATIV_STR(x) x.string()
#define WsFromNativ(x) utf82ws(x)
extern void OutputDebugString(const wchar_t* pOut);
extern void OutputDebugStringA(const char* pOut);
auto _kbhit = []() -> int
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
};
#endif

//using namespace std;
using namespace tinyxml2;

const static unordered_map<wstring, int> arMethoden = { { L"PROPFIND", 0 },{ L"PROPPATCH", 1 },{ L"MKCOL", 2 },{ L"COPY", 3 },{ L"MOVE", 4 },{ L"DELETE", 5 },{ L"LOCK", 6 },{ L"UNLOCK", 7 },{ L"PUT", 8 },{ L"OPTIONS", 9 },{ L"GET", 10 }, { L"HEAD", 11 } };

template<class A, class B>
struct tree : std::vector<struct tree<A, B>>
{
    A Element;
    B Value;
};

std::wstring mbs2ws(const std::string src)
{
    const size_t mbslen = mbstowcs(NULL, src.c_str(), 0);
    if (mbslen == (size_t) -1) {
        return L"";
    }

    std::wstring dst(mbslen, 0);

    if (mbstowcs(&dst[0], src.c_str(), mbslen) == (size_t) -1) {
        return L"";
    }

    return dst;
}

std::string ws2mbs(const std::wstring src)
{
    const size_t wslen = wcstombs(NULL, src.c_str(), 0);
    if (wslen == (size_t) -1) {
        return "";
    }

    std::string dst(wslen, 0);

    if (wcstombs(&dst[0], src.c_str(), wslen) == (size_t) -1) {
        return "";
    }

    return dst;

}

std::wstring utf82ws(const std::string src)
{
    const std::string prev_loc = std::setlocale(LC_ALL, nullptr);
    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::wstring strRet = mbs2ws(src);

    std::setlocale(LC_ALL, prev_loc.c_str());

    return strRet;
}

std::string ws2utf8(const std::wstring src)
{
    const std::string prev_loc = std::setlocale(LC_ALL, nullptr);
    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::string strRet = ws2mbs(src);

    std::setlocale(LC_ALL, prev_loc.c_str());

    return strRet;
}

wstring url_decode(const string& strSrc)
{
    wstring ws;
    const size_t nLen = strSrc.size();
    wchar_t wch = 0;
    int nAnzParts = 0;
    for (size_t n = 0; n < nLen; ++n)
    {
        int chr = strSrc.at(n);
        if ('%' == chr)
        {
            if (n + 2 >= nLen || isxdigit(strSrc.at(n + 1)) == 0 || isxdigit(strSrc.at(n + 2)) == 0)
            {
                return ws;
            }
            const char Nipple1 = strSrc.at(n + 1) - (strSrc.at(n + 1) <= '9' ? '0' : (strSrc.at(n + 1) <= 'F' ? 'A' : 'a') - 10);
            const char Nipple2 = strSrc.at(n + 2) - (strSrc.at(n + 2) <= '9' ? '0' : (strSrc.at(n + 2) <= 'F' ? 'A' : 'a') - 10);
            chr = 16 * Nipple1 + Nipple2;
            n += 2;

            if (chr < 0x7f)
                ws += static_cast<wchar_t>(chr);
            else if (chr >= 0x80 && chr <= 0xBF)
            {
                chr &= 0x3F;
                if (nAnzParts-- == 2)
                    chr = chr << 6;
                wch |= chr;
                if (nAnzParts == 0)
                    ws += wch;
            }
            else if (chr > 0xC0 && chr <= 0xDF)
            {
                wch = (chr & 0x1F) << 6;
                nAnzParts = 1;
            }
            else if (chr > 0xE0 && chr <= 0xEF)
            {
                wch = (chr & 0xF) << 12;
                nAnzParts = 2;
            }
        }
        else
            ws += static_cast<wchar_t>(strSrc.at(n));
    }

    return ws;
}

string url_encode(const string& value)
{
    stringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
    {
        const string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if ((c > 32 && c < 127 && isalnum(c)) || c == L'-' || c == L'_' || c == L'.' || c == L'~' || c == L'/' || c == L':' || c == L'?' || c == L'&')
        {
            escaped << static_cast<unsigned char>(c);
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }

    return escaped.str();
}

tuple<size_t, size_t, size_t> FindRedirectMarker(wstring strA, wstring strB)
{
/*    const static wregex rx(L"/");
    vector<wstring> tokenA(wsregex_token_iterator(begin(strA), end(strA), rx, -1), wsregex_token_iterator());
    vector<wstring> tokenB(wsregex_token_iterator(begin(strB), end(strB), rx, -1), wsregex_token_iterator());
    size_t nStrt = 0;
    for (size_t n = 0; n < min(tokenA.size(), tokenB.size()); ++n)
    {
        if (tokenA[n] != tokenB[n])
        {
            size_t nEndA = nStrt + tokenA[n].size();
            size_t nEndB = nStrt + tokenB[n].size();
            for (size_t m = n + 1; m < min(tokenA.size(), tokenB.size()); ++m)
            {
                if (tokenA[m] == tokenB[m])
                    return make_tuple(nStrt, nEndA, nEndB);
                nEndA = 1 + tokenA[m].size();
                nEndB = 1 + tokenB[m].size();
            }
            return make_tuple(nStrt, nEndA, nEndB);
        }
        nStrt += 1 + tokenA[n].size();
    }
    return make_tuple(0, 0, 0);*/
    const size_t nPos = strA.find(strB);
    return make_tuple(0, nPos, 0);
}

void XmlIterate(const XMLElement* elm, tree<string, string>& treeXml, deque<pair<string, string>> queXmlNs = {}, string strAktNs = string())
{
    for (const XMLAttribute* attr = elm->FirstAttribute(); attr; attr = attr->Next())
    {
        const string pAttrName = attr->Name() != nullptr ? attr->Name() : "";
        const string pAttrValue = attr->Value() != nullptr ? attr->Value() : "";
        //OutputDebugStringA(string("AttributName: " + pAttrName + ", AttribureValue: " + pAttrValue + "\r\n").c_str());

        if (pAttrName.substr(0, 6) == "xmlns:")
            queXmlNs.emplace_front(pAttrName.substr(6), pAttrValue);
        else if (pAttrName.substr(0, 5) == "xmlns")
        {
            queXmlNs.emplace_front(pAttrName.substr(5), pAttrValue);
            strAktNs = queXmlNs.front().second;
        }
    }

    string pElemName = elm->Name() != nullptr ? elm->Name() : "";
    const string pElemValue = elm->GetText() != nullptr ? elm->GetText() : "";
    const size_t nPos = pElemName.find_first_of(":");
    if (nPos != string::npos)
    {
        auto itNs = find_if(begin(queXmlNs), end(queXmlNs), [pElemName, nPos](auto item) { return item.first == pElemName.substr(0, nPos) ? true : false; });
        if (itNs != queXmlNs.end())
            pElemName.replace(0, nPos + 1, itNs->second);
    }
    else
        pElemName = strAktNs + pElemName;

    //OutputDebugStringA(string("ElementName: " + pElemName + ", ElementValue: " + pElemValue + "\r\n").c_str());
    treeXml.Element = pElemName;
    treeXml.Value = pElemValue;

    for (const XMLElement* node = elm->FirstChildElement(); node; node = node->NextSiblingElement())
    {
        treeXml.push_back({});
        XmlIterate(node, treeXml.back(), queXmlNs, strAktNs);
    }

}

int DoAction(const wstring& strModulePath, const map<wstring, wstring>& mapEnvList, ostream& streamOut, istream& streamIn)
{
    vector<pair<string, string>> vHeaderList = { { make_pair("X-Powerd-By", "webdav-cgi/0.1"), make_pair("DAV", "1,2") } };

    unordered_map<wstring, int>::const_iterator itMethode;
    if (find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return (pr.first == L"REQUEST_METHOD" && (itMethode = arMethoden.find(pr.second)) != end(arMethoden)) ? true : false;  }) == end(mapEnvList))
    {
//#if defined(_WIN32) || defined(_WIN64)
        auto itMeth = mapEnvList.find(L"REQUEST_METHOD");
        OutputDebugString(wstring(L"Unknown method called" + (itMeth == end(mapEnvList) ? wstring() : itMeth->second) + L"\r\n").c_str());
//#endif
        vHeaderList.push_back(make_pair("status", to_string(400)));
        vHeaderList.push_back(make_pair("Content-Length", "0"));

        string strHeader;
        for (const auto& itItem : vHeaderList)
            strHeader += itItem.first + ": " + itItem.second + "\n";
        strHeader += "\n";

        streamOut << strHeader;
        return 0;
    }

    uint64_t nContentSize = 0;
    wstring strHost;
    wstring strPath;
    wstring strRootPath;
    wstring strRequestUri;
    string strHttp("http");
    wstring strAuthorization;

    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"HTTP_CONTENT_LENGTH" || pr.first == L"CONTENT_LENGTH"); }); it != end(mapEnvList))
        nContentSize = stoll(it->second);
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"HTTP_HOST"); }); it != end(mapEnvList))
        strHost = it->second;
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"PATH_INFO"); }); it != end(mapEnvList))
    {
        strPath = url_decode(ws2mbs(it->second));
        OutputDebugString(wstring(L"PATH_INFO = " + strPath + L"\r\n").c_str());
    }
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"DOCUMENT_ROOT"); }); it != end(mapEnvList))
        strRootPath = it->second;
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"DAV_ROOT"); }); it != end(mapEnvList))
    {
        strRootPath = it->second;
        replace(begin(strRootPath), end(strRootPath), L'\\', L'/');
        OutputDebugString(wstring(L"DAV_ROOT = " + strRootPath + L"\r\n").c_str());
    }
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return (pr.first == L"REQUEST_URI"); }); it != end(mapEnvList))
        strRequestUri = url_decode(ws2mbs(it->second));
    if (find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"HTTPS");  }) != end(mapEnvList))
        strHttp += "s";
    if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"HTTP_AUTHORIZATION"); }); it != end(mapEnvList))
    {
        strAuthorization = it->second;
        OutputDebugString(wstring(L"HTTP_AUTHORIZATION = " + strAuthorization + L"\r\n").c_str());
    }

    if (strPath.empty() == true)    // Kommt vor, wenn über FastCgi aufgerufen wird
    {
        if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [](const auto& pr) { return (pr.first == L"SCRIPT_FILENAME");  }); it != end(mapEnvList))
        {
            strPath = it->second/*url_decode(ConvertToByte(pr.second))*/;
            OutputDebugString(wstring(L"SCRIPT_FILENAME = " + strPath + L"\r\n").c_str());
        }
    }
    OutputDebugString(wstring(L"ROOT+PATH = " + strRootPath + strPath + L"\r\n").c_str());

    tuple<size_t, size_t, size_t> nDiff = FindRedirectMarker(strRequestUri, strPath);
    // Sollte die RequestURI <> dem Pfad sein ist wahrscheinlich ein Redirect Marker am Anfang
    //wstring strReqOffstr = strRequestUri;
    //size_t nPos = strReqOffstr.find(strPath);
    //strReqOffstr.erase(nPos != string::npos ? nPos : 0);

    static std::map<std::wstring, std::vector<std::tuple<std::string, std::string, std::string>>> mapAccess;
    static time_t tLastWriteTime = 0;

    {
        struct _stat64 stFileInfo;
        const int iRet = _wstat64(FN_STR((strModulePath + L"/.access")).c_str(), &stFileInfo);
        //OutputDebugStringA(std::string("ret: " + std::to_string(iRet) + ", difftime:" + std::to_string(difftime(stFileInfo.st_mtime, tLastWriteTime)) + "\r\n").c_str());
        if (iRet != 0 || difftime(stFileInfo.st_mtime, tLastWriteTime) > 0)
        {
            fstream fin(FN_STR(strModulePath + L"/.access"), ios::in);
            if (fin.is_open() == true)
            {
                mapAccess.clear();
                std::map<std::wstring, std::vector<std::tuple<std::string, std::string, std::string>>>::iterator itLastDir = mapAccess.end();
                while (fin.eof() == false)
                {
                    std::string strLine;
                    std::getline(fin, strLine);
                    if (strLine.empty() == false)
                    {
                        OutputDebugStringA(std::string("fin reads: " + strLine + "\r\n").c_str());
                        if (isspace(strLine[0]) == false)
                        {
                            bool bExcluded = false;
                            replace(begin(strLine), end(strLine), '\\', '/');
                            const size_t nPos = strLine.find(",");
                            if (nPos != string::npos)
                            {
                                string strTmp = strLine.substr(nPos + 1);
                                strLine.erase(nPos);
                                std::transform(begin(strTmp), end(strTmp), begin(strTmp), [](char c) noexcept { return static_cast<char>(::tolower(c)); });
                                bExcluded = strTmp.find("independent") != string::npos;
                            }
                            auto prResult = mapAccess.emplace(mbs2ws(strLine), std::vector<std::tuple<std::string, std::string, std::string>>());
                            if (prResult.second == true)
                            {
                                itLastDir = prResult.first;
                                if (bExcluded == false)
                                    itLastDir->second.push_back({ "","","" });
                            }
                        }
                        else
                        {
                            const static regex s_rx("^\\s+([^:\\s]+)\\s*:\\s*([^,\\s]+)\\s*,\\s*([^\\s]+)\\s*");
                            std::smatch mr;
                            if (std::regex_search(strLine, mr, s_rx) == true && mr.size() == 4 && mr[1].matched == true && mr[2].matched == true && mr[3].matched == true)
                            {
                                std::string strTmp(mr[1]);
                                std::transform(begin(strTmp), end(strTmp), begin(strTmp), [](char c) noexcept { return static_cast<char>(::tolower(c)); });
                                itLastDir->second.push_back(make_tuple(strTmp, mr[2], mr[3]));
                            }
                        }
                    }
                    else
                        itLastDir = mapAccess.end();
                }
                fin.close();

                tLastWriteTime = stFileInfo.st_mtime;

                // Fix vererbung
                for (auto& itAccess : mapAccess)
                {   // Do we have a marker entry, all empty
                    if (get<0>(*itAccess.second.begin()) == "" && get<1>(*itAccess.second.begin()) == "" && get<2>(*itAccess.second.begin()) == "")
                    {
                        itAccess.second.erase(itAccess.second.begin()); // delete marker entry
                        std::wstring strPath = itAccess.first;
                        while (strPath.size() > strRootPath.size() && strPath != strRootPath)
                        {
                            const size_t nPos = strPath.rfind(L"/");
                            strPath.erase(nPos + 1 == strPath.size() ? nPos : nPos + 1);
                            const auto& itFound = mapAccess.find(strPath);
                            if (itFound != mapAccess.end() && *itFound != itAccess)
                            {
                                for (auto& it : itFound->second)
                                {
                                    if (get<0>(it) != "" || get<1>(it) != "" || get<2>(it) != "")
                                        itAccess.second.push_back(it);
                                }
                            }
                        }
                    }
                }

            }
        }
    }

    auto fnSendNotAuthenticate = [&]()
    {
        vHeaderList.push_back(make_pair("status", to_string(401)));
        vHeaderList.push_back(make_pair("WWW-Authenticate", "Basic realm=\"DavAccess\""));

        string strHeader;
        for (const auto& itItem : vHeaderList)
            strHeader += itItem.first + ": " + itItem.second + "\n";
        strHeader += "\n";

        streamOut << strHeader;
    };

    // Find if Item path is configured, if yes use this
    auto fnGetDirAccess = [&](std::wstring strPath) -> std::map<std::wstring, std::vector<std::tuple<std::string, std::string, std::string>>>::iterator
    {
        replace(begin(strPath), end(strPath), L'\\', L'/');
        auto itFound = mapAccess.find(strPath);
        while (itFound == mapAccess.end() && strPath != strRootPath)
        {
            size_t nPos = strPath.rfind(L"/");
            if (nPos == std::string::npos) break;
            if (strPath[strPath.size() - 1] != L'/' && strPath.size() - 1 > nPos) ++nPos;
            strPath.erase(nPos);
            itFound = mapAccess.find(strPath);
        }

        return itFound;
    };

    auto itFound = fnGetDirAccess(strRootPath + strPath);
    if (itFound == mapAccess.end())
    {
        fnSendNotAuthenticate();
        return 0;
    }

    string strCredential;
    const string::size_type nPosSpace = strAuthorization.find(L' ');
    if (nPosSpace != string::npos)
    {
        if (strAuthorization.substr(0, nPosSpace) == L"Basic")
        {
            strCredential = Base64::Decode(ws2mbs(strAuthorization.substr(nPosSpace + 1)));
            OutputDebugStringA(string("UserName&Password = " + strCredential + "\r\n").c_str());

            const size_t nPos = strCredential.find(":");
            std::transform(begin(strCredential), begin(strCredential) + nPos, begin(strCredential), [](char c) noexcept { return static_cast<char>(::tolower(c)); });
        }
    }

    auto fnGetReadWriteAccess = [&strCredential](std::map<std::wstring, std::vector<std::tuple<std::string, std::string, std::string>>>::iterator& iMapAccess) -> int
    {
        int iRet = 0;
        for (auto itTpCred : iMapAccess->second)
        {
            if (get<0>(itTpCred) + ":" + get<1>(itTpCred) == strCredential)
            {
                if (get<2>(itTpCred) == "r" || get<2>(itTpCred) == "rw" || get<2>(itTpCred) == "wr")
                    iRet = 1;
                if (get<2>(itTpCred) == "w" || get<2>(itTpCred) == "rw" || get<2>(itTpCred) == "wr")
                    iRet |= 2;
                OutputDebugString(wstring(L"ZUGRISSRECHTE: PFAD = " + iMapAccess->first + L", USER = " + wstring(begin(strCredential), end(strCredential)) + L", RECHTE = " + to_wstring(iRet) + L"\r\n").c_str());
                break;
            }
        }
        return iRet;
    };

    const int iRW = fnGetReadWriteAccess(itFound);
    if (iRW == 0)
    {
        fnSendNotAuthenticate();
        return 0;
    }

    auto fnBuildRespons = [](XMLNode* parent, const wstring& strRef, bool bIsCollection, vector<pair<string, wstring>> vPrItems)
    {
        tinyxml2::XMLDocument& doc = *parent->GetDocument();
        XMLElement* respons = doc.NewElement("D:response");
        respons->SetAttribute("xmlns:lp1", "DAV:");
        parent->InsertEndChild(respons);

        XMLElement* href = doc.NewElement("D:href");
        //newEle->SetText(string("http://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost) + strRef).c_str());
        href->SetText(ws2utf8(strRef).c_str());

        respons->InsertEndChild(href);
        XMLNode* propstat = respons->InsertEndChild(doc.NewElement("D:propstat"));

        XMLNode* prop = propstat->InsertEndChild(doc.NewElement("D:prop"));

        XMLElement* ResType = doc.NewElement("lp1:resourcetype");
        prop->InsertEndChild(ResType);

        if (bIsCollection == true)
        {
            XMLElement* Col = doc.NewElement("D:collection");
            ResType->InsertEndChild(Col);
        }

        for (auto& prItem : vPrItems)
        {
            XMLElement* newEle = doc.NewElement(prItem.first.c_str());
            if (prItem.second.empty() == false)
                newEle->SetText(ws2mbs(prItem.second).c_str());
            prop->InsertEndChild(newEle);
        }

        XMLElement* supportedlock = doc.NewElement("D:supportedlock");
        prop->InsertEndChild(supportedlock);

        XMLElement* lockentry = doc.NewElement("D:lockentry");
        supportedlock->InsertEndChild(lockentry);

        lockentry->InsertEndChild(doc.NewElement("D:lockscope"))->InsertEndChild(doc.NewElement("D:exclusive"));
        lockentry->InsertEndChild(doc.NewElement("D:locktype"))->InsertEndChild(doc.NewElement("D:write"));

        lockentry = doc.NewElement("D:lockentry");
        supportedlock->InsertEndChild(lockentry);

        lockentry->InsertEndChild(doc.NewElement("D:lockscope"))->InsertEndChild(doc.NewElement("D:shared"));
        lockentry->InsertEndChild(doc.NewElement("D:locktype"))->InsertEndChild(doc.NewElement("D:write"));

        prop->InsertEndChild(doc.NewElement("D:lockdiscovery"));

        XMLElement* status = doc.NewElement("D:status");
        status->SetText("HTTP/1.1 200 OK");
        propstat->InsertEndChild(status);
    };

    auto fnGetPathProp = [&strPath](fs::path pItem, vector<pair<string, wstring>>& vPropertys) -> wstring
    {
        error_code ec;
        struct _stat64 stFileInfo;

        std::wstring strFName(WsFromNativ(NATIV_STR(pItem.filename())));// wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(pItem.string());
        const int iRet = _wstat64(NATIV_STR(pItem).c_str(), &stFileInfo);

        if ((fs::is_directory(pItem, ec) == true && ec == error_code())
        || (iRet == 0 && stFileInfo.st_mode & S_IFDIR))
        {
#if defined(_WIN32) || defined(_WIN64)
//OutputDebugString(wstring(L"-->" + strFName + L"<-->" + pItem.generic_wstring() + L"\r\n").c_str());
#endif
            if (strFName == L".")
                strFName = strPath;// strFName.clear();
            //else if (strFName.back() != L'/')
            //    strFName += L"/";
        }
        else
        {
//                vPropertys.emplace_back("D:getcontentlength", to_wstring(fs::file_size(pItem, ec)));
            vPropertys.emplace_back("lp1:getcontentlength", to_wstring(stFileInfo.st_size));
            // Calc ETag
            const wstring strEtag = mbs2ws(MD5(ConvertToByte(NATIV_STR(pItem)) + ":" + to_string(stFileInfo.st_mtime) + ":" + to_string(stFileInfo.st_size)).getDigest());
            vPropertys.emplace_back("lp1:getetag", strEtag);
        }
//            vPropertys.emplace_back("lp1:displayname", strFName.empty() == true ? L"/" : strFName);
//            auto in_time_t = chrono::system_clock::to_time_t(fs::last_write_time(pItem, ec));
        auto in_time_t = stFileInfo.st_mtime;
        wstringstream ss;
        ss << put_time(::gmtime(&in_time_t), L"%a, %d %b %Y %H:%M:%S GMT");  // "Thu, 08 Dec 2016 10:20:54 GMT"
        vPropertys.emplace_back("lp1:getlastmodified", ss.str());

        in_time_t = stFileInfo.st_ctime;
        wstringstream().swap(ss);
        ss << put_time(::gmtime(&in_time_t), L"%a, %d %b %Y %H:%M:%S GMT");  // "Thu, 08 Dec 2016 10:20:54 GMT"
        vPropertys.emplace_back("lp1:creationdate", ss.str());

        if (strPath == strFName)
            strFName.clear();
        return strFName;
    };

    XMLPrinter streamer(nullptr, true);
    //doc.SetBOM(true);
    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());               // <?xml version=\"1.0\" encoding=\"utf-8\" ?>
    XMLElement* newEle = doc.NewElement("D:multistatus");
    newEle->SetAttribute("xmlns:D", "DAV:");
    XMLNode* element = doc.InsertEndChild(newEle);

    error_code ec;
    int iStatus = 200;
    struct _stat64 stFileInfo;

    // https://msdn.microsoft.com/en-us/library/ms991960(v=exchg.65).aspx
    // https://msdn.microsoft.com/en-us/library/jj594347(v=office.12).aspx
    tree<string, string> treeXml;
OutputDebugString(wstring(itMethode->first + L"(" + to_wstring(itMethode->second) + L") -> " + strRootPath + L"|" + strPath + L"\r\n").c_str());
    switch (itMethode->second)
    {
    case 0: // PROPFIND
        if (nContentSize > 0)
        {
            stringstream ss;
            std::copy_n(istreambuf_iterator<char>(streamIn), nContentSize, ostreambuf_iterator<char>(ss));

            tinyxml2::XMLDocument xmlIn;
            xmlIn.Parse(ss.str().c_str(), ss.str().size());

            XmlIterate(xmlIn.RootElement(), treeXml);

            if (treeXml.Element == "DAV:propfind" && treeXml.front().Element == "DAV:prop")
            {
                for (auto it : treeXml.front())
                {
#if defined(_WIN32) || defined(_WIN64)
//                    OutputDebugStringA(string("Element: " + it.Element + "\r\n").c_str());
#endif
                }
            }
        }
//this_thread::sleep_for(chrono::milliseconds(30000));
/*{
    fs::path pFile(strRootPath + strPath);
    fs::file_status status = fs::status(pFile, ec);
    if (ec != error_code()) OutputDebugStringA(std::string("error_code = " + std::to_string(ec.value()) + "\n").c_str());
    if(fs::is_regular_file(status)) OutputDebugStringA(" is a regular file\n");
    if(fs::is_directory(status)) OutputDebugStringA(" is a directory\n");
    if(fs::is_block_file(status)) OutputDebugStringA(" is a block device\n");
    if(fs::is_character_file(status)) OutputDebugStringA(" is a character device\n");
    if(fs::is_fifo(status)) OutputDebugStringA(" is a named IPC pipe\n");
    if(fs::is_socket(status)) OutputDebugStringA(" is a named IPC socket\n");
    if(fs::is_symlink(status)) OutputDebugStringA(" is a symlink\n");
    if(!fs::exists(status)) OutputDebugStringA(" does not exist\n");

    status = fs::symlink_status(pFile, ec);
    if (ec != error_code()) OutputDebugStringA(std::string("error_code = " + std::to_string(ec.value()) + "\n").c_str());
    if(fs::is_regular_file(status)) OutputDebugStringA("link is a regular file\n");
    if(fs::is_directory(status)) OutputDebugStringA("link is a directory\n");
    if(fs::is_block_file(status)) OutputDebugStringA("link is a block device\n");
    if(fs::is_character_file(status)) OutputDebugStringA("link is a character device\n");
    if(fs::is_fifo(status)) OutputDebugStringA("link is a named IPC pipe\n");
    if(fs::is_socket(status)) OutputDebugStringA("link is a named IPC socket\n");
    if(fs::is_symlink(status)) OutputDebugStringA("link is a symlink\n");
    if(!fs::exists(status)) OutputDebugStringA("link does not exist\n");

    struct _stat64 stFileInfo;
    int iRet = _wstat64(pFile.string().c_str(), &stFileInfo);
    if (stFileInfo.st_mode & __S_IFDIR)
            {
        OutputDebugStringA("stat is a directory\n");
        for (auto& p : fs::directory_iterator(pFile))
        {
            OutputDebugString(std::wstring(p.path().wstring() + L"\n").c_str());
        }
    }
}*/
        if ((fs::is_directory(strRootPath + strPath, ec) == true && ec == error_code())
        || (_wstat64(FN_STR((strRootPath + strPath)).c_str(), &stFileInfo) == 0 && stFileInfo.st_mode & S_IFDIR))
        {
            //if (strPath.back() != L'/')
            //    strPath += L'/';
            if (strRequestUri.back() != L'/')
                strRequestUri += L'/';

            wstring strDepth;
            bool bNoRoot = false;
            if (find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return (pr.first == L"HTTP_DEPTH") ? strDepth = pr.second, true : false;  }) != end(mapEnvList))
            {
                const size_t nPos = strDepth.find(L",noroot");
                if (nPos != string::npos)
                    bNoRoot = true, strDepth.erase(nPos);
            }

            if (bNoRoot == false)
            {
                vector<pair<string, wstring>> vPropertys;
                fnGetPathProp(strRootPath + strPath, vPropertys);
                fnBuildRespons(element, strRequestUri, true, vPropertys);
            }

            if (strDepth == L"1" || strDepth == L"infinity")
            {
                for (auto& p : fs::directory_iterator(strRootPath + strPath, ec))
                {
                    if (p.path() == L".")
                        continue;
                    vector<pair<string, wstring>> vPropertys;
                    const wstring strRef = strRequestUri + fnGetPathProp(p.path(), vPropertys);
                    const bool bIsDir = ((fs::is_directory(p, ec) == true && ec == error_code())
                                    || (_wstat64(NATIV_STR(p.path()).c_str(), &stFileInfo) == 0 && stFileInfo.st_mode & S_IFDIR));
                    fnBuildRespons(element, strRef, bIsDir/*S_ISDIR(stFileInfo.st_mode)*/, vPropertys);
                }
                if (ec != error_code())
                {
                    OutputDebugStringA(std::string(ec.message()+ "\n").c_str());
                }
            }

            iStatus = 207;
            vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=utf-8"));
            //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
            doc.Print(&streamer);
        }
        else
        {
            const fs::path pFile(strRootPath + strPath);
            if (fs::exists(pFile, ec) == true && ec == error_code())
            {
                vector<pair<string, wstring>> vPropertys;
                fnGetPathProp(pFile, vPropertys);
                //vPropertys.emplace_back("D:displayname", pFile.filename().wstring());
                //vPropertys.emplace_back("D:getcontentlength", to_wstring(fs::file_size(pFile, ec)));
                //auto in_time_t = chrono::system_clock::to_time_t(fs::last_write_time(pFile, ec));
                //wstringstream ss;
                //ss << put_time(::gmtime(&in_time_t), L"%a, %d %b %Y %H:%M:%S GMT");  // "Thu, 08 Dec 2016 10:20:54 GMT"
                //vPropertys.emplace_back("D:getlastmodified", ss.str());

                fnBuildRespons(element, strRequestUri, false, vPropertys);
                iStatus = 207;
                vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=utf-8"));
                //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
                doc.Print(&streamer);
            }
            else
            {
                iStatus = 404;
                //vHeaderList.push_back(make_pair("Content-Length", "0"));
            }
        }
        break;

    case 1: // PROPPATCH
        iStatus = 403;
        if ((iRW & 2) == 2)
        {
            {
                XMLElement* pElemResp = doc.NewElement("D:response");
                pElemResp->SetAttribute("xmlns:Z", "urn:schemas-microsoft-com:");
                element->InsertEndChild(pElemResp);
                XMLElement* pElemHRef = doc.NewElement("D:href");
                //pElemHRef->SetText(string(strHttp + "://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strReqOffstr + strPath)).c_str());
                //string strRef(strHttp + "://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strRequestUri));
                const string strRef(ws2mbs(strRequestUri));
                pElemHRef->SetText(strRef.c_str());
                pElemResp->InsertEndChild(pElemHRef);

                tinyxml2::XMLDocument xmlIn;
                if (nContentSize > 0)
                {
                    stringstream ss;
                    std::copy_n(istreambuf_iterator<char>(streamIn), nContentSize, ostreambuf_iterator<char>(ss));
                    /*while (streamIn.eof() == false)
                    {
                        char caBuffer[4096];
                        size_t nRead = streamIn.read(caBuffer, 4096).gcount();
                        if (nRead > 0)
                            ss.write(caBuffer, nRead);
                        else
                            this_thread::sleep_for(chrono::milliseconds(10));
                    }*/

                    xmlIn.Parse(ss.str().c_str(), ss.str().size());

                    XmlIterate(xmlIn.RootElement(), treeXml);
                }

                function<bool(const string&, const string&)> fnApplyProperty = [&](const string& strProperty, const string& strValue) -> bool
                {   // https://msdn.microsoft.com/en-us/library/jj594347(v=office.12).aspx
                    if (strProperty == "Z:Win32CreationTime" || strProperty == "Z:Win32LastAccessTime" || strProperty == "Z:Win32LastModifiedTime")
                    {
                        struct tm tmTime;
                        stringstream ss(strValue);
                        ss >> get_time(&tmTime, "%a, %d %b %Y %H:%M:%S GMT");

                        struct _stat stFile;
                        if (_stat(ws2mbs(strRootPath + strPath).c_str(), &stFile) == 0) // Alles OK
                        {
                            struct _utimbuf utTime;
                            utTime.actime = stFile.st_atime;
                            utTime.modtime = stFile.st_mtime;

                            if (strProperty == "Z:Win32LastModifiedTime")
                                utTime.modtime = _mkgmtime(&tmTime);
                            if (strProperty == "Z:Win32LastAccessTime")
                                utTime.actime = _mkgmtime(&tmTime);

                            if (_utime(ws2mbs(strRootPath + strPath).c_str(), &utTime) == 0)
                                return true;
                        }
                    }
                    else if (strProperty == "Z:Win32FileAttributes")
                    {
#if defined(_WIN32) || defined(_WIN64)
                        const DWORD dwAttr = stoi(strValue, 0, 16);
                        if (SetFileAttributes(wstring(strRootPath + strPath).c_str(), dwAttr) != 0)
                            return true;
#endif
                    }
                    return false;
                };

                if (treeXml.Element == "DAV:propertyupdate" && treeXml.front().Element == "DAV:set")
                {
                    for (auto itSet : treeXml.front())
                    {
                        if (itSet.Element == "DAV:prop")
                        {
                            for (auto itProp : itSet)
                            {
#if defined(_WIN32) || defined(_WIN64)
                                //                            OutputDebugStringA(string("Element: " + itProp.Element + ", Value: " + itProp.Value + "\r\n").c_str());
#endif
                            }
                        }
                    }
                }

                XMLElement* pElement = xmlIn.FirstChildElement("D:propertyupdate");
                if (pElement != nullptr)
                {
                    const XMLAttribute* pAttribut = pElement->FirstAttribute();
                    while (pAttribut)
                    {
                        pAttribut = pAttribut->Next();
                    }

                    pElement = pElement->FirstChildElement("D:set");
                    if (pElement != nullptr)
                    {
                        pElement = pElement->FirstChildElement("D:prop");
                        if (pElement != nullptr)
                        {
                            XMLElement* pElemPropStat = doc.NewElement("D:propstat");
                            XMLElement* pElemProp = doc.NewElement("D:prop");
                            pElemPropStat->InsertEndChild(pElemProp);

                            pElement = pElement->FirstChildElement();
                            while (pElement)
                            {
                                const char* pElemName = pElement->Name();
                                const char* pElemValue = pElement->GetText();
                                fnApplyProperty(pElemName, pElemValue);
                                pElemProp->InsertEndChild(doc.NewElement(pElemName));
                                pElement = pElement->NextSiblingElement();
                            }

                            XMLElement* pElemStatus = doc.NewElement("D:status");
                            pElemStatus->SetText("HTTP/1.1 200 Ok");
                            pElemPropStat->InsertEndChild(pElemStatus);

                            pElemResp->InsertEndChild(pElemPropStat);
                        }
                    }
                }
            }
            iStatus = 207;
            vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=utf-8"));
            //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
            doc.Print(&streamer);
        }
        break;

    case 2: // MKCOL
        iStatus = 403;
        if ((iRW & 2) == 2)
        {
            if (fs::create_directory(strRootPath + strPath, ec) == true && ec == error_code())
                iStatus = 201;
            else
                OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" create directory: " + strRootPath + strPath + L"\r\n").c_str());
        }
        break;

    case 3: // COPY
        iStatus = 403;
        if ((iRW & 1) == 1)
        {
            if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return pr.first == L"HTTP_DESTINATION"; }); it != end(mapEnvList))
            {
                // "destination", "http://thomas-pc/sample/test1/"
                const size_t nPos = it->second.find(strHost);
                if (nPos != string::npos)
                {
                    const fs::path src(wstring(strRootPath + strPath));
                    wstring strDst = url_decode(std::string(ws2mbs(it->second.substr(nPos + strHost.size()))));
                    //                        strDst.replace(get<0>(nDiff), get<1>(nDiff) - get<0>(nDiff), strPath.substr(get<0>(nDiff), get<2>(nDiff) - get<0>(nDiff)));
                    strDst.replace(get<0>(nDiff), get<1>(nDiff), L"");

                    //fs::path dst(regex_replace(wstring(strRootPath + url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())))), wregex(strReqOffstr), L"", regex_constants::format_first_only));
                    const fs::path dst(strRootPath + strDst);

                    auto itFoundDest = fnGetDirAccess(dst.wstring());
                    if (itFoundDest != mapAccess.end())
                    {
                        const int iRWDest = fnGetReadWriteAccess(itFoundDest);
                        if ((iRWDest & 2) == 2)
                        {
                            error_code ec;
                            fs::copy(src, dst, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
                            if (ec == error_code())
                                iStatus = 201;
                            else
                                OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" copy from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());
                        }
                    }
                }
            }
        }
        break;

    case 4: // MOVE
        iStatus = 403;  // Forbidden
        if ((iRW & 1) == 1)
        {
            if (const auto& it = find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return pr.first == L"HTTP_DESTINATION"; }); it != end(mapEnvList))
            {
                // "destination", "http://thomas-pc/sample/test1/"
                const size_t nPos = it->second.find(strHost);
                if (nPos != string::npos)
                {
                    const fs::path src(wstring(strRootPath + strPath));
                    wstring strDst = url_decode(ws2mbs(it->second.substr(nPos + strHost.size())));
                    //                        strDst.replace(get<0>(nDiff), get<1>(nDiff) - get<0>(nDiff), strPath.substr(get<0>(nDiff), get<2>(nDiff) - get<0>(nDiff)));
                    //OutputDebugString(wstring(L"redirect markers 1: " + to_wstring(get<0>(nDiff)) + L" 2: " + to_wstring(get<1>(nDiff))));
                    strDst.replace(get<0>(nDiff), get<1>(nDiff), L"");

                    //fs::path dst(regex_replace(wstring(strRootPath + url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())))), wregex(strReqOffstr), L"", regex_constants::format_first_only));
                    const fs::path dst(strRootPath + strDst);
                    OutputDebugString(wstring(L"move from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());

                    auto itFoundDest = fnGetDirAccess(dst.wstring());
                    if (itFoundDest != mapAccess.end())
                    {
                        const int iRWDest = fnGetReadWriteAccess(itFoundDest);
                        if ((iRWDest & 2) == 2)
                        {
                            fs::rename(src, dst, ec);
                            if (ec == error_code())
                                iStatus = 201;
                            else
                                OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" move from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());
                        }
                    }
                }
            }
        }
        break;

    case 5: // DELETE
        iStatus = 403;  // Forbidden
        if ((iRW & 2) == 2)
        {
            iStatus = 404;  // Not found
            if (fs::exists(strRootPath + strPath, ec) && ec == error_code())
            {
                iStatus = 423;  // Locked
                if ((fs::is_directory(strRootPath + strPath, ec) == true && ec == error_code())
                    || (_wstat64(FN_STR((strRootPath + strPath)).c_str(), &stFileInfo) == 0 && stFileInfo.st_mode & S_IFDIR))
                {
                    if (fs::remove_all(fs::path(strRootPath + strPath), ec) != static_cast<uintmax_t>(-1) && ec == error_code())
                        iStatus = 204;
                    else if (fs::remove(fs::path(strRootPath + strPath), ec) == true && ec == error_code())
                        iStatus = 204;
                    else
                        OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" removing path\r\n").c_str());
                }
                else
                {
                    if (fs::remove(fs::path(strRootPath + strPath), ec) == true && ec == error_code())
                        iStatus = 204;
                    else
                        OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" removing file\r\n").c_str());
                }

                if (iStatus != 204)
                {
                    XMLElement* respons = doc.NewElement("D:response");
                    respons->SetAttribute("xmlns:lp1", "DAV:");
                    element->InsertEndChild(respons);

                    XMLElement* href = doc.NewElement("D:href");
                    href->SetText(ws2mbs(strRequestUri).c_str());
                    respons->InsertEndChild(href);

                    XMLElement* status = doc.NewElement("D:status");
                    status->SetText("HTTP/1.1 423 Locked");
                    respons->InsertEndChild(status);

                    XMLElement* error = doc.NewElement("D:error");
                    error->SetText("<d:lock-token-submitted/>");
                    respons->InsertEndChild(error);

                    iStatus = 207;
                    vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=utf-8"));
                    doc.Print(&streamer);
                }
            }
        }
        break;

    case 6: // LOCK
        {
    //this_thread::sleep_for(chrono::milliseconds(30000));
            string strOwner("unknown");
            tinyxml2::XMLDocument xmlIn;
            if (nContentSize > 0)
            {
                stringstream ss;
                std::copy_n(istreambuf_iterator<char>(streamIn), nContentSize, ostreambuf_iterator<char>(ss));
                xmlIn.Parse(ss.str().c_str(), ss.str().size());
                XmlIterate(xmlIn.RootElement(), treeXml);
            }

            XMLElement* pElement = xmlIn.FirstChildElement("D:lockinfo");
            if (pElement != nullptr)
            {
                pElement = pElement->FirstChildElement("D:owner");
                if (pElement != nullptr)
                {
                    pElement = pElement->FirstChildElement("D:href");
                    if (pElement != nullptr)
                        strOwner = pElement->GetText();
                }
            }

            uint64_t nNextLockToken = 1000;
            const int fd = _wopen(FN_STR((strModulePath + L"/.davlock")).c_str(), O_CREAT | O_RDWR, S_IREAD | S_IWRITE);
            //fstream fin(FN_STR(strModulePath + L"/.davlock"), ios::in | ios::binary);
            //if (fin.is_open() == true)
            if (fd != -1)
            {
#if defined(_WIN32) || defined(_WIN64)
                OVERLAPPED overlapvar = { 0 };
                LockFileEx((HANDLE)_get_osfhandle(fd), LOCKFILE_EXCLUSIVE_LOCK, 0, UINT_MAX, 0, &overlapvar);
#else
                struct  flock   param;
                param.l_type = F_WRLCK;
                param.l_whence = SEEK_SET;
                param.l_start = 0; //start of the file
                param.l_len = 0; // 0 means end of the file
                fcntl(fd, F_SETLKW, &param);
#endif
                const unsigned long lLen = _lseek(fd, 0L, SEEK_END);
                _lseek(fd, 0L, SEEK_SET);

                const unique_ptr<char[]> caBuf = make_unique<char[]>(lLen + 1);
                memset(caBuf.get(), 0, lLen + 1);
                _read(fd, caBuf.get(), lLen);

                //stringstream ss;
                //copy(istreambuf_iterator<char>(fin), istreambuf_iterator<char>(), ostreambuf_iterator<char>(ss));
                //fin.close();

                //nNextLockToken = stoi(ss.str());
                string strTemp(caBuf.get());
                if (strTemp.empty() == false)
                    nNextLockToken = stoi(strTemp);
                nNextLockToken++;
                strTemp = to_string(nNextLockToken);

                //fstream fout(FN_STR(strModulePath + L"/.davlock"), ios::out | ios::binary);
                //if (fout.is_open() == true)
                //{
                    //string strTmp = to_string(nNextLockToken);
                    //fout.write(strTmp.c_str(), strTmp.size());
                    //fout.close();
                //}
                _lseek(fd, 0L, SEEK_SET);
                _write(fd, strTemp.c_str(), static_cast<unsigned int>(strTemp.size()));

#if defined(_WIN32) || defined(_WIN64)
                UnlockFileEx((HANDLE)_get_osfhandle(fd), 0, 0, 0, &overlapvar);
#else
                param.l_type = F_UNLCK;
                param.l_whence = SEEK_SET;
                param.l_start = 0; //start of the file
                param.l_len = 0; // 0 means end of the file
                fcntl(fd, F_SETLKW, &param);
#endif
                _close(fd);
            }

            tinyxml2::XMLDocument xmlOut;
            xmlOut.InsertEndChild(xmlOut.NewDeclaration());               // <?xml version=\"1.0\" encoding=\"utf-8\" ?>
            XMLElement* newEle = xmlOut.NewElement("D:prop");
            newEle->SetAttribute("xmlns:D", "DAV:");
            XMLNode* element = xmlOut.InsertEndChild(newEle);

            XMLElement* lockDisc = xmlOut.NewElement("D:lockdiscovery");
            element->InsertEndChild(lockDisc);
            XMLElement* actLock = xmlOut.NewElement("D:activelock");
            lockDisc->InsertEndChild(actLock);

            XMLElement* lockTyp = xmlOut.NewElement("D:locktype");
            actLock->InsertEndChild(lockTyp);
            lockTyp->InsertEndChild(xmlOut.NewElement("D:write"));

            XMLElement* lockScope = xmlOut.NewElement("D:lockscope");
            actLock->InsertEndChild(lockScope);
            lockScope->InsertEndChild(xmlOut.NewElement("D:exclusive"));

            XMLElement* lockdepth = xmlOut.NewElement("D:depth");
            lockdepth->SetText("infinity");
            actLock->InsertEndChild(lockdepth);

            XMLElement* owner = xmlOut.NewElement("D:owner");
            actLock->InsertEndChild(owner);
            XMLElement* href = xmlOut.NewElement("D:href");
            href->SetText(strOwner.c_str());
            owner->InsertEndChild(href);

            XMLElement* timeout = xmlOut.NewElement("D:timeout");
            timeout->SetText("Second-3600");
            actLock->InsertEndChild(timeout);

            XMLElement* lockTock = xmlOut.NewElement("D:locktoken");
            actLock->InsertEndChild(lockTock);
            //XMLElement* lockRoot = xmlOut.NewElement("D:lockroot");
            //lockTock->InsertEndChild(lockRoot);
            href = xmlOut.NewElement("D:href");
            href->SetText(to_string(nNextLockToken).c_str());
            lockTock->InsertEndChild(href);//lockRoot->InsertEndChild(href);

            iStatus = 200;
            vHeaderList.push_back(make_pair("Lock-Token", to_string(nNextLockToken)));
            vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=utf-8"));
            //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
            xmlOut.Print(&streamer);
        }
        break;

    case 7: // UNLOCK
        iStatus = 204;  // No Content - Normal success response
        break;

    case 8: // PUT
        iStatus = 403;  // Forbidden
//OutputDebugString(wstring(L"PUT ContentSize: " + to_wstring(nContentSize) + L"\r\n").c_str());
        if ((iRW & 2) == 2)
        {
            fstream fout(fs::path(strRootPath + strPath), ios::out | ios::binary);
            if (fout.is_open() == true)
            {
                if (nContentSize > 0)
                {
                    //while (streamIn.eof() == false)
                    copy(istreambuf_iterator<char>(streamIn), istreambuf_iterator<char>(), ostreambuf_iterator<char>(fout));
                }

                fout.close();
//OutputDebugString(wstring(L"PUT Datei geschlossen\r\n").c_str());
                iStatus = 201;
                //vHeaderList.push_back(make_pair("Location", string(strHttp + "://" + url_encode(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strReqOffstr + strPath))).c_str()));
                vHeaderList.push_back(make_pair("Location" , string(strHttp + "://" + url_encode(ws2mbs(strHost + strRequestUri))).c_str()));
                //vHeaderList.push_back(make_pair("Content-Length", "0"));
            }
            else
                OutputDebugString(wstring(L"PUT: Error opening destination file: " + std::to_wstring(errno) + L"\r\n").c_str());
        }
        break;

    case 9: // OPTION
        vHeaderList.push_back(make_pair("Allow", "OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, COPY, MOVE, MKCOL, PROPFIND, PROPPATCH, LOCK, UNLOCK, ORDERPATCH"));
        //vHeaderList.push_back(make_pair("Content-Length", "0"));
        iStatus = 200;
        break;

    case 10:// GET
        iStatus = 403;  // Forbidden
        if ((iRW & 1) == 1)
        {
            iStatus = 200;

            int iRet = _wstat64(FN_STR((strRootPath + strPath)).c_str(), &stFileInfo);
            fstream fin(FN_STR(strRootPath + strPath), ios::in | ios::binary);
            if (iRet == 0 && fin.is_open() == true)
            {
                stringstream strLastModTime; strLastModTime << put_time(::gmtime(&stFileInfo.st_mtime), "%a, %d %b %Y %H:%M:%S GMT");
                // Calc ETag
                string strEtag = MD5(ws2mbs(strRootPath + strPath) + ":" + to_string(stFileInfo.st_mtime) + ":" + to_string(stFileInfo.st_size)).getDigest();

                //vHeaderList.push_back(make_pair("Content-Type", strMineType));
                vHeaderList.push_back(make_pair("Last-Modified", strLastModTime.str()));
                //vHeaderList.push_back(make_pair("Cache-Control", "private, must-revalidate"));
                vHeaderList.push_back(make_pair("ETag", strEtag));
                vHeaderList.push_back(make_pair("Content-Length", to_string(stFileInfo.st_size)));

                string strHeader;
                for (const auto& itItem : vHeaderList)
                    strHeader += itItem.first + ": " + itItem.second + "\r\n";
                strHeader += "\r\n";

                streamOut << strHeader;

                //copy(istreambuf_iterator<char>(fin), istreambuf_iterator<char>(), ostreambuf_iterator<char>(cout));
                unique_ptr<char> pBuf(new char[32768]);
                while (fin.eof() == false)
                {
                    streamsize nRead = fin.read(pBuf.get(), 32768).gcount();
                    if (nRead > 0)
                        streamOut.write(pBuf.get(), nRead);
                }
                streamOut.flush();

                fin.close();

                vHeaderList.clear();
            }
            else
            {
                iStatus = 404;
                if(fin.is_open() == true)
                    fin.close();
            }
        }
        break;

    case 11:// HEAD
        iStatus = 403;  // Forbidden
        if ((iRW & 1) == 1)
        {
            //this_thread::sleep_for(chrono::milliseconds(30000));
            iStatus = 200;
            //int iRet = _wstat64(FN_STR((strRootPath + strPath)).c_str(), &stFileInfo);
            //if (iRet == 0 && S_ISREG(stFileInfo.st_mode) == true)
            if (fs::is_regular_file(strRootPath + strPath, ec) == true && ec == error_code())
            {
                fs::file_time_type ftime = fs::last_write_time(strRootPath + strPath, ec);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
                //std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime); // assuming system_clock

                //stringstream strLastModTime; strLastModTime << put_time(::gmtime(&stFileInfo.st_mtime), "%a, %d %b %Y %H:%M:%S GMT");
                    stringstream strLastModTime; strLastModTime << put_time(::gmtime(&cftime), "%a, %d %b %Y %H:%M:%S GMT");
                // Calc ETag
                size_t nFSize = fs::file_size(strRootPath + strPath, ec);
                string strEtag = MD5(ws2mbs(strRootPath + strPath) + ":" + to_string(cftime) + ":" + to_string(nFSize)).getDigest();

                //vHeaderList.push_back(make_pair("Content-Type", strMineType));
                vHeaderList.push_back(make_pair("Last-Modified", strLastModTime.str()));
                //vHeaderList.push_back(make_pair("Cache-Control", "private, must-revalidate"));
                vHeaderList.push_back(make_pair("ETag", strEtag));

/*                string strHeader;
                for (const auto& itItem : vHeaderList)
                    strHeader += itItem.first + ": " + itItem.second + "\r\n";
                //strHeader += "\r\n";

                streamOut << strHeader;
                streamOut.flush();

                vHeaderList.clear();*/
            }
            else
                iStatus = 404;
        }
        break;

    default:
        iStatus = 412;
        OutputDebugString(L"Unknown methode\r\n");
    }

#if defined(_WIN32) || defined(_WIN64)
    //string strTmp(streamer.CStr(), streamer.CStrSize());
    //uint32_t nLen1 = strlen(streamer.CStr());
    //uint32_t nLen2 = streamer.CStrSize() - 1;
    //if (nLen1 != nLen2)
    //    OutputDebugString(L"Wrong len\r\n");
#endif
    //OutputDebugString(wstring(L"  Status: " + to_wstring(iStatus) + L"\r\n").c_str());
    if (iStatus != 200)
        vHeaderList.push_back(make_pair("status", to_string(iStatus)));

    if (vHeaderList.size() > 0)
    {
        if (streamer.CStrSize() > 1)    // the Size from CStrSize is including the trailing 0
            vHeaderList.push_back(make_pair("Content-Length", to_string(streamer.CStrSize() - 1)));
        else
            vHeaderList.push_back(make_pair("Content-Length", "0"));

        string strHeader;
        for (const auto& itItem : vHeaderList)
            strHeader += itItem.first + ": " + itItem.second + "\r\n";
        strHeader += "\r\n";
//#if defined(_WIN32) || defined(_WIN64)
//OutputDebugStringA(strHeader.c_str());
//#endif
        streamOut << strHeader;
    }

    if (streamer.CStrSize() > 1)
    {
            streamOut << streamer.CStr();
//#if defined(_WIN32) || defined(_WIN64)
//OutputDebugStringA(streamer.CStr()); OutputDebugStringA("\r\n");
//#endif
    }
//OutputDebugStringA("--------------------\n");
    return 1;

    return 0;
}

int main(int argc, const char* argv[], char **envp)
{
    SrvParam SrvPara;
    wstring strModulePath = wstring(FILENAME_MAX, 0);
#if defined(_WIN32) || defined(_WIN64)
    if (GetModuleFileName(NULL, &strModulePath[0], FILENAME_MAX) > 0)
        strModulePath.erase(strModulePath.find_last_of(L'\\') + 1); // Sollte der Backslash nicht gefunden werden wird der ganz String gelöscht

    SrvPara.szDspName = L"´Webdav Server";
    SrvPara.szDescribe = L"Webdav fastcgi Server by Thomas Hauck";
#else
    string strTmpPath(FILENAME_MAX, 0);
    if (readlink(string("/proc/" + to_string(getpid()) + "/exe").c_str(), &strTmpPath[0], FILENAME_MAX) > 0)
        strTmpPath.erase(strTmpPath.find_last_of('/'));
    strModulePath = wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(strTmpPath) + L"/";
#endif

    if (getenv("REQUEST_METHOD") == nullptr)
    {
        SrvPara.szSrvName = L"WebdavServ";

        deque<FastCgiServer> m_vServers;

        SrvPara.fnStartCallBack = [&strModulePath, &m_vServers]()
        {
            ConfFile& conf = ConfFile::GetInstance(strModulePath + L"webdav.cfg");
            vector<wstring> vListen = conf.get(L"Listen");
            if (vListen.empty() == true)
                vListen.push_back(L"127.0.0.1"), vListen.push_back(L"::1");

            map<string, vector<wstring>> mIpPortCombi;
            for (const auto& strListen : vListen)
            {
                string strIp(ws2mbs(strListen));
                vector<wstring> vPort = move(conf.get(L"Listen", strListen));
                if (vPort.empty() == true)
                    vPort.push_back(L"9010");
                for (const auto& strPort : vPort)
                {   // Default Werte setzen
                    uint16_t nPort = 0;
                    try { nPort = static_cast<uint16_t>(stoi(strPort)); }
                    catch (const std::exception& /*ex*/) {}

                    if (mIpPortCombi.find(strIp) == end(mIpPortCombi))
                        mIpPortCombi.emplace(strIp, vector<wstring>({ strPort }));
                    else
                        mIpPortCombi.find(strIp)->second.push_back(strPort);
                    if (find_if(begin(m_vServers), end(m_vServers), [&nPort, &strIp](auto& CgiServer) { return CgiServer.GetPort() == nPort && CgiServer.GetBindAdresse() == strIp ? true : false; }) != end(m_vServers))
                        continue;
                    m_vServers.emplace_back(strIp, nPort, [&](const PARAMETERLIST& lstParam, ostream& streamOut, istream& streamIn)
                        {
                            map<wstring, wstring> mapEnvList;
                            for (const auto& itParam : lstParam)
                                mapEnvList.emplace(mbs2ws(itParam.first), utf82ws(itParam.second));
                            return DoAction(strModulePath, mapEnvList, streamOut, streamIn);
                        });
                }
            }

            // Server starten und speichern
            for (auto& CgiServer : m_vServers)
            {
                if (CgiServer.Start() == false)
                    wcout << L"Error starting server, error no.:" << CgiServer.GetError() << endl;
            }
        };

        SrvPara.fnStopCallBack = [&m_vServers]()
        {
            for (auto& CgiServer : m_vServers)
                CgiServer.Stop();
        };

        SrvPara.fnSignalCallBack = [&strModulePath]()
        {
        };

        return ServiceMain(argc, argv, SrvPara);
    }

#if defined(_WIN32) || defined(_WIN64)
    _setmode(_fileno(stdin), O_BINARY);
    _setmode(_fileno(stdout), O_BINARY);
#endif

    map<wstring, wstring> mapEnvList;
    for (char **env = envp; *env != 0; ++env)
    {
        string strTmp = *env;
        wstring wstrTmp = wstring(begin(strTmp), end(strTmp));

        size_t nPos = wstrTmp.find_first_of('=');
        mapEnvList.emplace(wstrTmp.substr(0, nPos), wstrTmp.substr(nPos + 1));
    }

    //this_thread::sleep_for(chrono::milliseconds(30000));
    //#if defined(_WIN32) || defined(_WIN64)
    //#if defined(_DEBUG)
    //wstring strDebug;
    //for (auto& itHeader : mapEnvList)
    //    strDebug += itHeader.first + L": " + itHeader.second + L"\r\n";
    //strDebug += L"\r\n";
    //OutputDebugString(strDebug.c_str());
    //#endif
    //#endif

    return DoAction(strModulePath, mapEnvList, cout, cin);
}
