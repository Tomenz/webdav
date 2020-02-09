// webdav.cpp : Definiert die exportierten Funktionen für die DLL-Anwendung.
//
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

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "tinyxml2/tinyxml2.h"
#include "FastCgi/FastCgi.h"

using namespace std;
using namespace tinyxml2;

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
#define ConvertToByte(x) wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(x)
#include <conio.h>
#include <io.h>
#define FN_STR(x) x
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#else
//#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#define ConvertToByte(x) wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(x)
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
#define FN_STR(x) wstring_convert<codecvt_utf8<wchar_t>, wchar_t>().to_bytes(x)
void OutputDebugString(const wchar_t* pOut)
{   // mkfifo /tmp/dbgout  ->  tail -f /tmp/dbgout
    int fdPipe = open("/tmp/dbgout", O_WRONLY | O_NONBLOCK);
    if (fdPipe >= 0)
    {
        wstring strTmp(pOut);
        write(fdPipe, ConvertToByte(strTmp).c_str(), strTmp.size());
        close(fdPipe);
    }
}
void OutputDebugStringA(const char* pOut)
{   // mkfifo /tmp/dbgout  ->  tail -f /tmp/dbgout
    int fdPipe = open("/tmp/dbgout", O_WRONLY | O_NONBLOCK);
    if (fdPipe >= 0)
    {
        string strTmp(pOut);
        write(fdPipe, strTmp.c_str(), strTmp.size());
        close(fdPipe);
    }
}
#endif

#if defined(_WIN32) || defined(_WIN64)
#ifdef _DEBUG
#ifdef _WIN64
#pragma comment(lib, "x64/Debug/socketlib64d")
#pragma comment(lib, "x64/Debug/CommonLib")
#else
#pragma comment(lib, "Debug/socketlib32d")
#pragma comment(lib, "Debug/CommonLib")
#endif
#else
#ifdef _WIN64
#pragma comment(lib, "x64/Release/socketlib64")
#pragma comment(lib, "x64/Release/CommonLib")
#else
#pragma comment(lib, "Release/socketlib32")
#pragma comment(lib, "Release/CommonLib")
#endif
#endif
#endif

#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

const static unordered_map<wstring, int> arMethoden = { { L"PROPFIND", 0 },{ L"PROPPATCH", 1 },{ L"MKCOL", 2 },{ L"COPY", 3 },{ L"MOVE", 4 },{ L"DELETE", 5 },{ L"LOCK", 6 },{ L"UNLOCK", 7 },{ L"PUT", 8 },{ L"OPTIONS", 9 },{ L"GET", 10 }, { L"HEAD", 11 } };

template<class A, class B>
struct tree : std::vector<struct tree<A, B>>
{
    A Element;
    B Value;
};

wstring url_decode(const string& strSrc)
{
    wstring ws;
    size_t nLen = strSrc.size();
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
            char Nipple1 = strSrc.at(n + 1) - (strSrc.at(n + 1) <= '9' ? '0' : (strSrc.at(n + 1) <= 'F' ? 'A' : 'a') - 10);
            char Nipple2 = strSrc.at(n + 2) - (strSrc.at(n + 2) <= '9' ? '0' : (strSrc.at(n + 2) <= 'F' ? 'A' : 'a') - 10);
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
        string::value_type c = (*i);

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
    const static wregex rx(L"/");
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
            return make_tuple(nStrt, -1, -1);
        }
        nStrt += 1 + tokenA[n].size();
    }
    return make_tuple(0, 0, 0);
}

void XmlIterate(const XMLElement* elm, tree<string, string>& treeXml, deque<pair<string, string>> queXmlNs = {}, string strAktNs = string())
{
    for (const XMLAttribute* attr = elm->FirstAttribute(); attr; attr = attr->Next())
    {
        string pAttrName = attr->Name() != nullptr ? attr->Name() : "";
        string pAttrValue = attr->Value() != nullptr ? attr->Value() : "";
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
    string pElemValue = elm->GetText() != nullptr ? elm->GetText() : "";
    size_t nPos = pElemName.find_first_of(":");
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
    if (find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"REQUEST_METHOD" && (itMethode = arMethoden.find(pr.second)) != end(arMethoden)) ? true : false;  }) != end(mapEnvList))
    {
        uint64_t nContentSize = 0;
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"HTTP_CONTENT_LENGTH" || pr.first == L"CONTENT_LENGTH") ? nContentSize = stoll(pr.second), true : false; });

        tinyxml2::XMLDocument doc;
        XMLPrinter streamer(nullptr, true);
        //doc.SetBOM(true);
        doc.InsertEndChild(doc.NewDeclaration());               // <?xml version=\"1.0\" encoding=\"utf-8\" ?>
        XMLElement* newEle = doc.NewElement("D:multistatus");
        newEle->SetAttribute("xmlns:D", "DAV:");
        XMLNode* element = doc.InsertEndChild(newEle);

        wstring strHost;
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"HTTP_HOST") ? strHost = pr.second, true : false;  });
        wstring strPath;
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"PATH_INFO") ? strPath = url_decode(ConvertToByte(pr.second)), true : false;  });
        wstring strRootPath;
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"DOCUMENT_ROOT") ? strRootPath = pr.second, true : false;  });
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"DAV_ROOT") ? strRootPath = pr.second, true : false;  });
        wstring strRequestUri;
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"REQUEST_URI") ? strRequestUri = url_decode(ConvertToByte(pr.second)), true : false;  });
        string strHttp("http");
        find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"HTTPS") ? strHttp += "s", true : false;  });

        if (strPath.empty() == true)    // Kommt vor, wenn über FastCgi aufgerufen wird
            find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr) { return (pr.first == L"SCRIPT_FILENAME") ? strPath = url_decode(ConvertToByte(pr.second)), true : false;  });

        tuple<size_t, size_t, size_t> nDiff = FindRedirectMarker(strRequestUri, strPath);
        // Sollte die RequestURI <> dem Pfad sein ist wahrscheinlich ein Redirect Marker am Anfang
        //wstring strReqOffstr = strRequestUri;
        //size_t nPos = strReqOffstr.find(strPath);
        //strReqOffstr.erase(nPos != string::npos ? nPos : 0);

        auto fnBuildRespons = [&](XMLNode* parent, const wstring& strRef, bool bIsCollection, vector<pair<string, wstring>> vPrItems)
        {
            parent = parent->InsertEndChild(doc.NewElement("D:response"));
            XMLElement* newEle = doc.NewElement("D:href");
            //newEle->SetText(string("http://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost) + strRef).c_str());
            newEle->SetText(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRef).c_str());

            parent->InsertEndChild(newEle);
            parent = parent->InsertEndChild(doc.NewElement("D:propstat"));

            XMLNode* node = parent->InsertEndChild(doc.NewElement("D:prop"));

            XMLElement* ResType = doc.NewElement("D:resourcetype");
            node->InsertEndChild(ResType);
            if (bIsCollection == true)
            {
                XMLElement* Col = doc.NewElement("D:collection");
                ResType->InsertEndChild(Col);
            }

            for (auto& prItem : vPrItems)
            {
                newEle = doc.NewElement(prItem.first.c_str());
                if (prItem.second.empty() == false)
                    newEle->SetText(ConvertToByte(prItem.second).c_str());
                node->InsertEndChild(newEle);
            }

            newEle = doc.NewElement("D:status");
            newEle->SetText("HTTP/1.1 200 OK");
            parent->InsertEndChild(newEle);
        };

        auto fnGetPathProp = [&](fs::path pItem, vector<pair<string, wstring>>& vPropertys) -> wstring
        {
//            error_code ec;
            struct _stat64 stFileInfo;
            int iRet = _wstat64(FN_STR(pItem.wstring()).c_str(), &stFileInfo);

            wstring strFName(pItem.filename().wstring());
//            if (fs::is_directory(pItem))
            if (S_ISDIR(stFileInfo.st_mode))
            {
#if defined(_WIN32) || defined(_WIN64)
OutputDebugString(wstring(L"-->" + strFName + L"<-->" + pItem.generic_wstring() + L"\r\n").c_str());
#endif
                if (strFName == L".")
                    strFName = strPath;// strFName.clear();
                //else if (strFName.back() != L'/')
                //    strFName += L"/";
            }
            else
            {
//                vPropertys.emplace_back("D:getcontentlength", to_wstring(fs::file_size(pItem, ec)));
                vPropertys.emplace_back("D:getcontentlength", to_wstring(stFileInfo.st_size));
                // Calc ETag
                string strToHash = wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(pItem.wstring()) + ":" + to_string(stFileInfo.st_mtime) + ":" + to_string(stFileInfo.st_size);
                md5::md5_t HashValue(strToHash.c_str(), strToHash.size());
                strToHash.reserve(33);
                HashValue.get_string(&strToHash[0]);
                wstring strEtag = wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(strToHash.substr(0, 32));
                vPropertys.emplace_back("D:getetag", strEtag);
            }
            vPropertys.emplace_back("D:displayname", strFName.empty() == true ? L"/" : strFName);
//            auto in_time_t = chrono::system_clock::to_time_t(fs::last_write_time(pItem, ec));
            auto in_time_t = stFileInfo.st_mtime;
            wstringstream ss;
            ss << put_time(::gmtime(&in_time_t), L"%a, %d %b %Y %H:%M:%S GMT");  // "Thu, 08 Dec 2016 10:20:54 GMT"
            vPropertys.emplace_back("D:getlastmodified", ss.str());

            in_time_t = stFileInfo.st_ctime;
            ss.swap(wstringstream());
            ss << put_time(::gmtime(&in_time_t), L"%a, %d %b %Y %H:%M:%S GMT");  // "Thu, 08 Dec 2016 10:20:54 GMT"
            vPropertys.emplace_back("D:creationdate", ss.str());

            if (strPath == strFName)
                strFName.clear();
            return strFName;
        };

        error_code ec;
        int iStatus = 200;
        struct _stat64 stFileInfo;

        // https://msdn.microsoft.com/en-us/library/ms991960(v=exchg.65).aspx
        // https://msdn.microsoft.com/en-us/library/jj594347(v=office.12).aspx
        tree<string, string> treeXml;
OutputDebugString(wstring(itMethode->first + L"(" + to_wstring(itMethode->second) + L") -> " + strRootPath + strPath + L"\r\n").c_str());
        switch (itMethode->second)
        {
        case 0: // PROPFIND
            if (nContentSize > 0)
            {
                stringstream ss;
                copy(istreambuf_iterator<char>(streamIn), istreambuf_iterator<char>(), ostreambuf_iterator<char>(ss));

                tinyxml2::XMLDocument xmlIn;
                xmlIn.Parse(ss.str().c_str(), ss.str().size());

                XmlIterate(xmlIn.RootElement(), treeXml);

                if (treeXml.Element == "DAV:propfind" && treeXml.front().Element == "DAV:prop")
                {
                    for (auto it : treeXml.front())
                    {
#if defined(_WIN32) || defined(_WIN64)
                        OutputDebugStringA(string("Element: " + it.Element + "\r\n").c_str());
#endif
                    }
                }
            }
//this_thread::sleep_for(chrono::milliseconds(30000));

            _wstat64(FN_STR(wstring(strRootPath + strPath)).c_str(), &stFileInfo);
//            if (fs::is_directory(strRootPath + strPath) == true)
            if (S_ISDIR(stFileInfo.st_mode))
            {
                //if (strPath.back() != L'/')
                //    strPath += L'/';
                if (strRequestUri.back() != L'/')
                    strRequestUri += L'/';

                wstring strDepth;
                bool bNoRoot = false;
                find_if(begin(mapEnvList), end(mapEnvList), [&](const auto& pr) { return (pr.first == L"HTTP_DEPTH") ? strDepth = pr.second, true : false;  });
                size_t nPos = strDepth.find(L",noroot");
                if (nPos != string::npos)
                    bNoRoot = true, strDepth.erase(nPos);

                if (bNoRoot == false)
                {
                    vector<pair<string, wstring>> vPropertys;
                    fnGetPathProp(strRootPath + strPath, vPropertys);
                    fnBuildRespons(element, strRequestUri, true, vPropertys);
                }

                if (strDepth == L"1" || strDepth == L"infinity")
                {
                    for (auto& p : fs::directory_iterator(strRootPath + strPath))
                    {
                        if (p.path() == L".")
                            continue;
                        vector<pair<string, wstring>> vPropertys;
                        wstring strRef = strRequestUri + fnGetPathProp(p.path(), vPropertys);
//                        fnBuildRespons(element, strRef, fs::is_directory(p.status()), vPropertys);
                        _wstat64(FN_STR(p.path().wstring()).c_str(), &stFileInfo);
                        fnBuildRespons(element, strRef, S_ISDIR(stFileInfo.st_mode), vPropertys);
                    }
                }

                iStatus = 207;
                vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=\"utf-8\""));
                //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
                doc.Print(&streamer);
            }
            else
            {
                fs::path pFile(strRootPath + strPath);
                if (fs::exists(pFile) == true)
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
                    vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=\"utf-8\""));
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
        {
            newEle->SetAttribute("xmlns:Z", "urn:schemas-microsoft-com:");

            XMLElement* pElemResp = doc.NewElement("D:response");
            element->InsertEndChild(pElemResp);
            XMLElement* pElemHRef = doc.NewElement("D:href");
            //pElemHRef->SetText(string(strHttp + "://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strReqOffstr + strPath)).c_str());
            //string strRef(strHttp + "://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strRequestUri));
            string strRef(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRequestUri));
            pElemHRef->SetText(strRef.c_str());
            pElemResp->InsertEndChild(pElemHRef);


            tinyxml2::XMLDocument xmlIn;
            if (nContentSize > 0)
            {
                stringstream ss;
                copy(istreambuf_iterator<char>(streamIn), istreambuf_iterator<char>(), ostreambuf_iterator<char>(ss));
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
                    tm tmTime = { 0 };
                    stringstream ss(strValue);
                    ss >> get_time(&tmTime, "%a, %d %b %Y %H:%M:%S GMT");

                    struct _stat stFile;
                    if (_stat(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRootPath + strPath).c_str(), &stFile) == 0) // Alles OK
                    {
                        struct _utimbuf utTime;
                        utTime.actime = stFile.st_atime;
                        utTime.modtime = stFile.st_mtime;

                        if (strProperty == "Z:Win32LastModifiedTime")
                            utTime.modtime = _mkgmtime(&tmTime);
                        if (strProperty == "Z:Win32LastAccessTime")
                            utTime.actime = _mkgmtime(&tmTime);

                        if (_utime(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRootPath + strPath).c_str(), &utTime) == 0)
                            return true;
                    }
                }
                else if (strProperty == "Z:Win32FileAttributes")
                {
#if defined(_WIN32) || defined(_WIN64)
                    DWORD dwAttr = stoi(strValue, 0, 16);
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
                            OutputDebugStringA(string("Element: " + itProp.Element + ", Value: " + itProp.Value + "\r\n").c_str());
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
        vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=\"utf-8\""));
        //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
        doc.Print(&streamer);
        break;

        case 2: // MKCOL
            iStatus = 403;
            if (fs::create_directory(strRootPath + strPath) == true)
                iStatus = 201;
            break;

        case 3: // COPY
            iStatus = 403;
            find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr)
            {
                if (pr.first == L"HTTP_DESTINATION")
                {
                    // "destination", "http://thomas-pc/sample/test1/"
                    size_t nPos = pr.second.find(strHost);
                    if (nPos != string::npos)
                    {
                        fs::path src(wstring(strRootPath + strPath));
                        wstring strDst = url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())));
                        strDst.replace(get<0>(nDiff), get<1>(nDiff) - get<0>(nDiff), strPath.substr(get<0>(nDiff), get<2>(nDiff) - get<0>(nDiff)));

                        //fs::path dst(regex_replace(wstring(strRootPath + url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())))), wregex(strReqOffstr), L"", regex_constants::format_first_only));
                        fs::path dst(strRootPath + strDst);

                        error_code ec;
                        fs::copy(src, dst, fs::copy_options::overwrite_existing|fs::copy_options::recursive, ec);
                        if (ec.value() == 0)
                            iStatus = 201;
                        else
                            OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" copy from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());
                    }
                    return true;
                }
                return false;
            });
            break;

        case 4: // MOVE
            iStatus = 403;  // Forbidden
            find_if(begin(mapEnvList), end(mapEnvList), [&](auto pr)
            {
                if (pr.first == L"HTTP_DESTINATION")
                {
                    // "destination", "http://thomas-pc/sample/test1/"
                    size_t nPos = pr.second.find(strHost);
                    if (nPos != string::npos)
                    {
                        fs::path src(wstring(strRootPath + strPath));
                        wstring strDst = url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())));
                        strDst.replace(get<0>(nDiff), get<1>(nDiff) - get<0>(nDiff), strPath.substr(get<0>(nDiff), get<2>(nDiff) - get<0>(nDiff)));

                        //fs::path dst(regex_replace(wstring(strRootPath + url_decode(ConvertToByte(pr.second.substr(nPos + strHost.size())))), wregex(strReqOffstr), L"", regex_constants::format_first_only));
                        fs::path dst(strRootPath + strDst);
OutputDebugString(wstring(L"move from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());

                        error_code ec;
                        fs::rename(src, dst, ec);
//#if defined(_WIN32) || defined(_WIN64)
                        if (ec.value() == 0)
                            iStatus = 201;
                        else
                            OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" move from: " + src.wstring() + L" to: " + dst.wstring() + L"\r\n").c_str());
//                            OutputDebugStringA(string(ec.message() + "\r\n").c_str());
//#endif
                    }
                    return true;
                }
                return false;
            });
            break;

        case 5: // DELETE
            iStatus = 404;  // Forbidden
//            if (fs::remove_all(fs::path(strRootPath + strPath), ec) >= 0)
            if (_wremove(FN_STR(wstring(strRootPath + strPath)).c_str()) == 0)
                iStatus = 204;
            else
            {
//#if defined(_WIN32) || defined(_WIN64)
                OutputDebugString(wstring(L"Error " + to_wstring(ec.value()) + L" removing path\r\n").c_str());
//#endif
            }
            break;

        case 6: // LOCK
        {
//this_thread::sleep_for(chrono::milliseconds(30000));
            uint64_t nNextLockToken = 1000;
            int fd = _wopen(FN_STR((strModulePath + L"/.davlock")).c_str(), O_CREAT | O_RDWR, S_IREAD | S_IWRITE);

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
                unsigned long lLen = _lseek(fd, 0L, SEEK_END);
                _lseek(fd, 0L, SEEK_SET);

                unique_ptr<char[]> caBuf = make_unique<char[]>(lLen + 1);
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
            href->SetText(string(strHttp + "://" + wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strPath)).c_str());
            owner->InsertEndChild(href);

            XMLElement* lockTock = xmlOut.NewElement("D:locktoken");
            actLock->InsertEndChild(lockTock);
            XMLElement* lockRoot = xmlOut.NewElement("D:lockroot");
            lockTock->InsertEndChild(lockRoot);
            href = xmlOut.NewElement("D:href");
            href->SetText(to_string(nNextLockToken).c_str());
            lockRoot->InsertEndChild(href);

            iStatus = 200;
            vHeaderList.push_back(make_pair("Lock-Token", to_string(nNextLockToken)));
            vHeaderList.push_back(make_pair("Content-Type", "application/xml; charset=\"utf-8\""));
            //vHeaderList.push_back(make_pair("Cache-Control", "must-revalidate"));
            xmlOut.Print(&streamer);
        }
        break;

        case 7: // UNLOCK
            iStatus = 204;
            break;

        case 8: // PUT
        {
            iStatus = 403;  // Forbidden
OutputDebugString(wstring(L"PUT ContentSize: " + to_wstring(nContentSize) + L"\r\n").c_str());
            if (nContentSize > 0)
            {
                fstream fout(FN_STR(strRootPath + strPath), ios::out | ios::binary);
                if (fout.is_open() == true)
                {
                    //while (streamIn.eof() == false)
                        copy(istreambuf_iterator<char>(streamIn), istreambuf_iterator<char>(), ostreambuf_iterator<char>(fout));

                    fout.close();
OutputDebugString(wstring(L"PUT Datei geschlossen\r\n").c_str());
                }
                else
                    OutputDebugString(wstring(L"PUT Datei geschlossen\r\n").c_str());

                iStatus = 201;
                //vHeaderList.push_back(make_pair("Location", string(strHttp + "://" + url_encode(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strReqOffstr + strPath))).c_str()));
                vHeaderList.push_back(make_pair("Location", string(strHttp + "://" + url_encode(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strRequestUri))).c_str()));
                //vHeaderList.push_back(make_pair("Content-Length", "0"));
            }
            else if (nContentSize == 0)
            {
                fstream fout(FN_STR(strRootPath + strPath), ios::out);
                if (fout.is_open() == true)
                {
                    fout.close();
OutputDebugString(wstring(L"PUT Datei geschlossen\r\n").c_str());
                }
                else
                    OutputDebugString(wstring(L"PUT Datei nicht geoeffnet\r\n").c_str());
                iStatus = 201;
                //vHeaderList.push_back(make_pair("Location", string(strHttp + "://" + url_encode(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strReqOffstr + strPath))).c_str()));
                vHeaderList.push_back(make_pair("Location", string(strHttp + "://" + url_encode(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strHost + strRequestUri))).c_str()));
                //vHeaderList.push_back(make_pair("Content-Length", "0"));
            }
        }
        break;

        case 9: // OPTION
            vHeaderList.push_back(make_pair("Allow", "OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, COPY, MOVE, MKCOL, PROPFIND, PROPPATCH, LOCK, UNLOCK, ORDERPATCH"));
            //vHeaderList.push_back(make_pair("Content-Length", "0"));
            iStatus = 200;
            break;

        case 10:// GET
        {
            iStatus = 200;

			int iRet = _wstat64(FN_STR(wstring(strRootPath + strPath)).c_str(), &stFileInfo);
			fstream fin(FN_STR(strRootPath + strPath), ios::in | ios::binary);
            if (iRet == 0 && fin.is_open() == true)
            {
				stringstream strLastModTime; strLastModTime << put_time(::gmtime(&stFileInfo.st_mtime), "%a, %d %b %Y %H:%M:%S GMT");
                // Calc ETag
                string strEtag = md5(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRootPath + strPath) + ":" + to_string(stFileInfo.st_mtime) + ":" + to_string(stFileInfo.st_size));

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
		{
            iStatus = 200;
			int iRet = _wstat64(FN_STR(wstring(strRootPath + strPath)).c_str(), &stFileInfo);
			if (iRet == 0 && S_ISREG(stFileInfo.st_mode) == true)
			{
				stringstream strLastModTime; strLastModTime << put_time(::gmtime(&stFileInfo.st_mtime), "%a, %d %b %Y %H:%M:%S GMT");
                // Calc ETag
                string strEtag = md5(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(strRootPath + strPath) + ":" + to_string(stFileInfo.st_mtime) + ":" + to_string(stFileInfo.st_size));

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

        if (streamer.CStrSize() > 1)    // the Size from CStrSize is including the trailing 0
            vHeaderList.push_back(make_pair("Content-Length", to_string(streamer.CStrSize() - 1)));
        else
            vHeaderList.push_back(make_pair("Content-Length", "0"));

        if (vHeaderList.size() > 0)
        {
            string strHeader;
            for (const auto& itItem : vHeaderList)
                strHeader += itItem.first + ": " + itItem.second + "\r\n";
            strHeader += "\r\n";
//#if defined(_WIN32) || defined(_WIN64)
OutputDebugStringA(strHeader.c_str());
//#endif
            streamOut << strHeader;
        }

        if (streamer.CStrSize() > 1)
        {
            streamOut << streamer.CStr();
//#if defined(_WIN32) || defined(_WIN64)
OutputDebugStringA(streamer.CStr()); OutputDebugStringA("\r\n");
//#endif
        }
//OutputDebugStringA("--------------------\n");
        return 1;
    }
    else
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
    }

    return 0;
}

int main(int argc, const char* argv[], char **envp)
{
    wstring strModulePath = wstring(FILENAME_MAX, 0);
#if defined(_WIN32) || defined(_WIN64)
    if (GetModuleFileName(NULL, &strModulePath[0], FILENAME_MAX) > 0)
        strModulePath.erase(strModulePath.find_last_of(L'\\') + 1); // Sollte der Backslash nicht gefunden werden wird der ganz String gelöscht
#else
    string strTmpPath(FILENAME_MAX, 0);
    if (readlink(string("/proc/" + to_string(getpid()) + "/exe").c_str(), &strTmpPath[0], FILENAME_MAX) > 0)
        strTmpPath.erase(strTmpPath.find_last_of('/'));
    strModulePath = wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(strTmpPath) + L"/";
#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////////

    if (argc > 1 && string(argv[1]).compare("-s") == 0)
    {
        FastCgiServer fcgiServ([&](const PARAMETERLIST& lstParam, ostream& streamOut, istream& streamIn)
        {
            map<wstring, wstring> mapEnvList;
            for (const auto& itParam : lstParam)
                mapEnvList.emplace(wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(itParam.first), wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(itParam.second));
            return DoAction(strModulePath, mapEnvList, streamOut, streamIn);
        });
        fcgiServ.Start("127.0.0.1", 1994);

        //mutex mxBeenden;
        //unique_lock<mutex> lock(mxBeenden);
        //condition_variable m_cvBeenden;
        //m_cvBeenden.wait(lock, [&]() { return false; });

        wcout << L"gestartet" << endl;

        const wchar_t caZeichen[] = L"\\|/-";
        int iIndex = 0;
        while (_kbhit() == 0)
        {
            wcout << L'\r' << caZeichen[iIndex++] /*<< L"  Sockets:" << setw(3) << BaseSocket::s_atRefCount << L"  SSL-Pumpen:" << setw(3) << SslTcpSocket::s_atAnzahlPumps << L"  HTTP-Connections:" << setw(3) << nHttpCon*/ << flush;
            if (iIndex > 3) iIndex = 0;
            this_thread::sleep_for(chrono::milliseconds(100));
        }

        wcout << L"gestoppt" << endl;

        fcgiServ.Stop();
        return 0;
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
    wstring strDebug;
    for (auto& itHeader : mapEnvList)
        strDebug += itHeader.first + L": " + itHeader.second + L"\r\n";
    strDebug += L"\r\n";
    //OutputDebugString(strDebug.c_str());
    //#endif
    //#endif

    return DoAction(strModulePath, mapEnvList, cout, cin);
}
