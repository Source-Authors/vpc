// Copyright Valve Corporation, All rights reserved.
//
// Purpose: VPC

#include "vpc.h"

#ifdef STEAM
#include "tier1/utlintrusivelist.h"
#endif

#ifdef POSIX
#define _O_RDONLY O_RDONLY
#define _open open
#include <sys/errno.h>
#define _lseek lseek
#define _read read
#define _close close
#define _stat stat
#include <glob.h>
#else
#include "winlite.h"
#include <io.h>
#include <ShellAPI.h>
#endif

#ifdef OSX
#include <mach-o/dyld.h>
#endif

#include "tier0/memdbgon.h"

CXMLWriter::CXMLWriter() {
  m_fp = NULL;
  m_b2010Format = false;
}

bool CXMLWriter::Open(const char *pFilename, bool b2010Format) {
  m_FilenameString = pFilename;
  m_b2010Format = b2010Format;

  m_fp = fopen(pFilename, "wt");
  if (!m_fp) return false;

  if (b2010Format) {
    Write("\xEF\xBB\xBF<?xml version=\"1.0\" encoding=\"utf-8\"?>");
  } else {
    // 2005 format
    Write("<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\n");
  }

  return true;
}

void CXMLWriter::Close() {
  if (!m_fp) return;
  fclose(m_fp);

  Sys_CopyToMirror(m_FilenameString.Get());

  m_fp = NULL;
  m_FilenameString = NULL;
}

void CXMLWriter::PushNode(const char *pName) {
  Indent();

  char *pNewName = _strdup(pName);
  m_Nodes.Push(pNewName);

  fprintf(m_fp, "<%s%s\n", pName, m_Nodes.Count() == 2 ? ">" : "");
}

void CXMLWriter::PushNode(const char *pName, const char *pString) {
  Indent();

  char *pNewName = _strdup(pName);
  m_Nodes.Push(pNewName);

  fprintf(m_fp, "<%s%s%s>\n", pName, pString ? " " : "",
          pString ? pString : "");
}

void CXMLWriter::WriteLineNode(const char *pName, const char *pExtra,
                               const char *pString) {
  Indent();

  fprintf(m_fp, "<%s%s>%s</%s>\n", pName, pExtra ? pExtra : "", pString, pName);
}

void CXMLWriter::PopNode(bool bEmitLabel) {
  char *pName;
  m_Nodes.Pop(pName);

  Indent();
  if (bEmitLabel) {
    fprintf(m_fp, "</%s>\n", pName);
  } else {
    fprintf(m_fp, "/>\n");
  }

  free(pName);
}

void CXMLWriter::Write(const char *p) {
  if (m_fp) {
    Indent();
    fprintf(m_fp, "%s\n", p);
  }
}

struct XMLFixup_t {
  const char *m_pFrom;
  const char *m_pTo;
  bool m_b2010Only;
};

CUtlString CXMLWriter::FixupXMLString(const char *pInput) {
  // these tokens are not allowed in xml vcproj and be be escaped per msdev docs
  XMLFixup_t xmlFixups[] = {
      {"&", "&amp;", false},
      {"\"", "&quot;", false},
      {"\'", "&apos;", false},
      {"\n", "&#x0D;&#x0A;", false},
      {">", "&gt;", false},
      {"<", "&lt;", false},
      {"$(InputFileName)", "%(Filename)%(Extension)", true},
      {"$(InputName)", "%(Filename)", true},
      {"$(InputPath)", "%(FullPath)", true},
      {"$(InputDir)", "%(RootDir)%(Directory)", true},
  };

  bool bNeedsFixups = false;
  CUtlVector<bool> needsFixups;
  CUtlString outString;

  needsFixups.SetCount(static_cast<intp>(std::size(xmlFixups)));
  for (intp i = 0; i < static_cast<intp>(std::size(xmlFixups)); i++) {
    needsFixups[i] = false;

    if (!m_b2010Format && xmlFixups[i].m_b2010Only) continue;

    if (V_stristr(pInput, xmlFixups[i].m_pFrom)) {
      needsFixups[i] = true;
      bNeedsFixups = true;
    }
  }

  if (!bNeedsFixups) {
    outString = pInput;
  } else {
    int flip = 0;
    char bigBuffer[2][8192];
    V_strncpy(bigBuffer[flip], pInput, sizeof(bigBuffer[0]));

    for (intp i = 0; i < static_cast<intp>(std::size(xmlFixups)); i++) {
      if (!needsFixups[i]) continue;

      if (!V_StrSubst(bigBuffer[flip], xmlFixups[i].m_pFrom, xmlFixups[i].m_pTo,
                      bigBuffer[flip ^ 1], sizeof(bigBuffer[0]), false)) {
        g_pVPC->VPCError("XML overflow - Increase big buffer");
      }
      flip ^= 1;
    }
    outString = bigBuffer[flip];
  }

  return outString;
}

void CXMLWriter::Indent() {
  for (int i = 0; i < m_Nodes.Count(); i++) {
    if (m_b2010Format) {
      fprintf(m_fp, "  ");
    } else {
      fprintf(m_fp, "\t");
    }
  }
}

//	Sys_LoadFile
int Sys_LoadFile(const char *filename, void **bufferptr, bool bText) {
  *bufferptr = nullptr;

  if (!Sys_Exists(filename)) return -1;

  int flags = _O_RDONLY;

#if !defined(POSIX)
  flags |= (bText ? _O_TEXT : _O_BINARY);
#endif

  int handle{_open(filename, flags)};
  if (handle == -1)
    Sys_Error("Sys_LoadFile(): Error opening %s: %s", filename,
              strerror(errno));

  long length{_lseek(handle, 0, SEEK_END)};
  _lseek(handle, 0, SEEK_SET);
  char *buffer = (char *)malloc(length + 1);

  int bytesRead = _read(handle, buffer, length);
  if (!bText && bytesRead != length)
    Sys_Error("Sys_LoadFile(): read truncated failure");

  _close(handle);

  // text mode is truncated, add null for parsing
  buffer[bytesRead] = '\0';

  *bufferptr = (void *)buffer;

  return length;
}

//	Sys_LoadFileIntoBuffer
bool Sys_LoadFileIntoBuffer(const char *pchFileIn, CUtlBuffer &buf,
                            bool bText) {
  buf.SetBufferType(bText, bText);

#ifndef _WIN64
  struct stat statBuf;
  if (::stat(pchFileIn, &statBuf) != 0) return false;
#else
  struct _stat64 statBuf;
  if (::_stat64(pchFileIn, &statBuf) != 0) return false;
#endif

  buf.EnsureCapacity(statBuf.st_size + 1);
  if (!buf.IsValid()) return false;

  FILE *f{fopen(pchFileIn, "rb")};
  if (!f) return false;

  char *pBuffer = (char *)buf.Base();
  const decltype(statBuf.st_size) nBytesRead =
      statBuf.st_size * fread(pBuffer, statBuf.st_size, 1, f);

  fclose(f);

  buf.SeekPut(CUtlBuffer::SEEK_HEAD, nBytesRead);
  // terminate buffer without changing put size
  pBuffer[statBuf.st_size] = '\0';

  return nBytesRead == statBuf.st_size;
}

//	Sys_FileLength
long Sys_FileLength(const char *filename, bool bText) {
  if (filename) {
    int flags = _O_RDONLY;

#if !defined(POSIX)
    flags |= (bText ? _O_TEXT : _O_BINARY);
#endif

    int handle{_open(filename, flags)};
    // file does not exist
    if (handle == -1) return -1;

    long length{_lseek(handle, 0, SEEK_END)};

    _close(handle);

    return length;
  }

  return -1;
}

//	Sys_StripPath
//
// Removes path portion from a fully qualified name, leaving filename and
// extension.
void Sys_StripPath(const char *inpath, char *outpath) {
  const char *src{inpath + strlen(inpath)};

  while ((src != inpath) && (*(src - 1) != '\\') && (*(src - 1) != '/') &&
         (*(src - 1) != ':'))
    src--;

  strcpy(outpath, src);
}

//	Sys_Exists
//
//	Returns TRUE if file exists.
bool Sys_Exists(const char *filename) {
#ifdef _WIN32
  // dimhotepus: Fast way to check file exists.
  return ::GetFileAttributesA(filename) != INVALID_FILE_ATTRIBUTES;
#else
  if (FILE *test = fopen(filename, "rb")) {
    fclose(test);
    return true;
  }

  return false;
#endif
}

//	Sys_Touch
//
//	Returns TRUE if the file could be accessed for write
bool Sys_Touch(const char *filename) {
  if (FILE *test = fopen(filename, "wb")) {
    fclose(test);
    return true;
  }

  return false;
}

//	Sys_FileInfo
bool Sys_FileInfo(const char *pFilename, int64 &nFileSize, int64 &nModifyTime) {
  struct _stat statData;
  int rt = _stat(pFilename, &statData);

  if (rt != 0) return false;

  nFileSize = statData.st_size;
  nModifyTime = statData.st_mtime;

  return true;
}

// Ignores allowable trailing characters.
bool Sys_StringToBool(const char *pString) {
  if (!V_strnicmp(pString, "no", 2) || !V_strnicmp(pString, "off", 3) ||
      !V_strnicmp(pString, "false", 5) || !V_strnicmp(pString, "not set", 7) ||
      !V_strnicmp(pString, "disabled", 8) || !V_strnicmp(pString, "0", 1)) {
    // false
    return false;
  }

  if (!V_strnicmp(pString, "yes", 3) || !V_strnicmp(pString, "on", 2) ||
      !V_strnicmp(pString, "true", 4) || !V_strnicmp(pString, "set", 3) ||
      !V_strnicmp(pString, "enabled", 7) || !V_strnicmp(pString, "1", 1)) {
    // true
    return true;
  }

  // unknown boolean expression
  g_pVPC->VPCSyntaxError("Unknown boolean expression '%s'", pString);
}

bool Sys_ReplaceString(const char *pStream, const char *pSearch,
                       const char *pReplace, char *pOutBuff, int outBuffSize) {
  const char *pFind;
  const char *pStart = pStream;
  char *pOut = pOutBuff;
  intp len;
  bool bReplaced = false;

  while (true) {
    // find sub string
    pFind = V_stristr(pStart, pSearch);
    if (!pFind) {
      /// end of string
      len = V_strlen(pStart);
      pFind = pStart + len;
      if (pOut + len - pOutBuff < outBuffSize) memcpy(pOut, pStart, len);
      pOut += len;
      break;
    } else {
      bReplaced = true;
    }

    // copy up to sub string
    len = pFind - pStart;
    if (pOut + len - pOutBuff < outBuffSize) memcpy(pOut, pStart, len);
    pOut += len;

    // substitute new string
    len = V_strlen(pReplace);
    if (pOut + len - pOutBuff < outBuffSize) memcpy(pOut, pReplace, len);
    pOut += len;

    // advance past sub string
    pStart = pFind + strlen(pSearch);
  }

  *pOut = '\0';

  return bReplaced;
}

// string match with wildcards.  '?' = match any char
bool Sys_StringPatternMatch(char const *pSrcPattern, char const *pString) {
  for (;;) {
    char nPat = *(pSrcPattern++);
    char nString = *(pString++);

    if (!((nPat == nString) || ((nPat == '?') && nString))) return false;
    if (!nString) return true;  // end of string
  }
}

bool Sys_EvaluateEnvironmentExpression(const char *pExpression,
                                       const char *pDefault, char *pOutBuff,
                                       int nOutBuffSize) {
  char *pEnvVarName = (char *)StringAfterPrefix(pExpression, "$env(");
  // not an environment specification
  if (!pEnvVarName) return false;

  char *pLastChar = &pEnvVarName[V_strlen(pEnvVarName) - 1];
  if (!*pEnvVarName || *pLastChar != ')') {
    g_pVPC->VPCSyntaxError("$env() must have a closing ')' in \"%s\"\n",
                           pExpression);
  }

  // get the contents of the $env( blah..blah ) expressions
  // handles expresions that could have whitepsaces
  g_pVPC->GetScript().PushScript(pExpression, pEnvVarName);
  const char *pToken = g_pVPC->GetScript().GetToken(false);
  g_pVPC->GetScript().PopScript();

  if (pToken && pToken[0]) {
    const char *pResolve = getenv(pToken);
    if (!pResolve) {
      // not defined, use default
      pResolve = pDefault ? pDefault : "";
    }

    V_strncpy(pOutBuff, pResolve, nOutBuffSize);
  }

  return true;
}

bool Sys_ExpandFilePattern(const char *pPattern,
                           CUtlVector<CUtlString> &vecResults) {
#if defined(_WIN32)
  char rgchPathPart[MAX_PATH];
  V_strncpy(rgchPathPart, pPattern, V_ARRAYSIZE(rgchPathPart));
  V_StripFilename(rgchPathPart);
  if (V_strlen(rgchPathPart))
    V_strncat(rgchPathPart, "\\", V_ARRAYSIZE(rgchPathPart));

  WIN32_FIND_DATA findData;
  HANDLE hFind = FindFirstFile(pPattern, &findData);
  if (hFind != INVALID_HANDLE_VALUE) {
    vecResults.AddToTail(
        CFmtStr("%s%s", rgchPathPart, findData.cFileName).Access());
    BOOL bMore = TRUE;

    while (bMore) {
      bMore = FindNextFile(hFind, &findData);
      if (bMore)
        vecResults.AddToTail(
            CFmtStr("%s%s", rgchPathPart, findData.cFileName).Access());
    }

    FindClose(hFind);
  }
#elif defined(POSIX)
  glob_t gr;
  if (glob(pPattern, 0, NULL, &gr) == 0) {
    for (size_t i = 0; i < gr.gl_pathc; i++) {
      vecResults.AddToTail(gr.gl_pathv[i]);
    }
    globfree(&gr);
  }
#else
#error
#endif
  return vecResults.Count() > 0;
}

bool Sys_GetExecutablePath(char *pBuf, int cbBuf) {
#if defined(_WIN32)
  return (0 != GetModuleFileNameA(NULL, pBuf, cbBuf));
#elif defined(OSX)
  uint32_t _nBuff = cbBuf;
  bool bSuccess = _NSGetExecutablePath(pBuf, &_nBuff) == 0;
  pBuf[cbBuf - 1] = '\0';
  return bSuccess;
#elif defined LINUX
  ssize_t nRead = readlink("/proc/self/exe", pBuf, cbBuf - 1);
  if (nRead != -1) {
    pBuf[nRead] = 0;
    return true;
  }

  pBuf[0] = 0;
  return false;
#else
#error Sys_GetExecutablePath
#endif
}

void Sys_CreatePath(const char *path) {
#if defined(_WIN32)
  char pFullPath[MAX_PATH];
  V_MakeAbsolutePath(pFullPath, sizeof(pFullPath), path);

  // If Sys_CreatePath is called with a filename, all is well.
  // If it is called with a folder name, it must have a trailing slash:
  if (!V_GetFileExtension(pFullPath))
    V_AppendSlash(pFullPath, sizeof(pFullPath));

  char *ptr;

  // skip past the drive path, but don't strip
  if (pFullPath[1] == ':') {
    ptr = strchr(pFullPath, '\\');
  } else {
    ptr = pFullPath;
  }

  while (ptr) {
    ptr = strchr(ptr + 1, '\\');

    if (ptr) {
      *ptr = '\0';

#if defined(_WIN32) || defined(WIN32)
      CreateDirectory(pFullPath, NULL);
#else
#error Sys_CreatePath: this mkdir is probably correct but has not been tested
      mkdir(pFullPath, 0777);
#endif

      *ptr = '\\';
    }
  }
#endif
}

// Given some arbitrary case filename, provides what the OS thinks it is.
// Windows specific.  Returns false if file cannot be resolved (i.e. does not
// exist).
bool Sys_GetActualFilenameCase(const char *pFilename, char *pOutputBuffer,
                               int nOutputBufferSize) {
#if defined(_WINDOWS)
  char filenameBuffer[MAX_PATH];
  V_strncpy(filenameBuffer, pFilename, sizeof(filenameBuffer));
  V_FixSlashes(filenameBuffer);
  V_RemoveDotSlashes(filenameBuffer);

  intp nFilenameLength = V_strlen(filenameBuffer);

  CUtlString actualFilename;

  // march along filename, resolving up to next seperator
  bool bAddSeparator = false;
  intp i = 0;
  while (i < nFilenameLength) {
    // cannot resolve these, emit as-is
    if (!V_strnicmp(filenameBuffer + i, ".\\", 2)) {
      i += 2;
      actualFilename += CUtlString(".\\");
      continue;
    }

    // cannot resolve these, emit as-is
    if (!V_strnicmp(filenameBuffer + i, "..\\", 3)) {
      i += 3;
      actualFilename += CUtlString("..\\");
      continue;
    }

    // skip until path separator
    while (i < nFilenameLength && filenameBuffer[i] != '\\') {
      ++i;
    }

    bool bFoundSeparator = (i < nFilenameLength);

    // truncate at separator, windows resolves each component in pieces
    filenameBuffer[i] = 0;

    SHFILEINFOA info = {};
    DWORD_PTR hr{SHGetFileInfoA(filenameBuffer, 0, &info, sizeof(info),
                                SHGFI_DISPLAYNAME)};
    if (hr != 0) {
      // reassemble based on actual component
      if (bAddSeparator) actualFilename += CUtlString("\\");

      actualFilename += CUtlString(info.szDisplayName);
    } else {
      return false;
    }

    // restore path separator
    if (bFoundSeparator) {
      filenameBuffer[i] = '\\';
    }

    ++i;
    bAddSeparator = true;
  }

  V_strncpy(pOutputBuffer, actualFilename.Get(), nOutputBufferSize);
  return true;
#else
  return false;
#endif
}

// Given some arbitrary case filename, determine if OS version matches.
bool Sys_IsFilenameCaseConsistent(const char *pFilename, char *pOutputBuffer,
                                  int nOutputBufferSize) {
  V_strncpy(pOutputBuffer, pFilename, nOutputBufferSize);

  // normalize the provided filename separators
  CUtlString filename = pFilename;
  V_FixSlashes(filename.Get());
  V_RemoveDotSlashes(filename.Get());

  if (!Sys_GetActualFilenameCase(filename.Get(), pOutputBuffer,
                                 nOutputBufferSize))
    return false;

  if (!V_strcmp(filename.Get(), pOutputBuffer)) return true;

  return false;
}

bool Sys_CopyToMirror(const char *pFilename) {
  if (!pFilename || !pFilename[0]) return false;

  const char *pMirrorPath = g_pVPC->GetOutputMirrorPath();
  if (!pMirrorPath || !pMirrorPath[0]) return false;

  char absolutePathToOriginal[MAX_PATH];
  if (V_IsAbsolutePath(pFilename)) {
    V_strncpy(absolutePathToOriginal, pFilename,
              sizeof(absolutePathToOriginal));
  } else {
    // need to determine where file resides for mirroring
    char currentDirectory[MAX_PATH] = {0};
    V_GetCurrentDirectory(currentDirectory, sizeof(currentDirectory));
    V_ComposeFileName(currentDirectory, pFilename, absolutePathToOriginal,
                      sizeof(absolutePathToOriginal));
  }

  if (!Sys_Exists(absolutePathToOriginal)) {
    g_pVPC->VPCWarning("Cannot mirror '%s', cannot resolve to expected '%s'",
                       pFilename, absolutePathToOriginal);
    return false;
  }

  const char *pTargetPath =
      StringAfterPrefix(absolutePathToOriginal, g_pVPC->GetSourcePath());
  if (!pTargetPath || !pTargetPath[0]) {
    g_pVPC->VPCWarning(
        "Cannot mirror '%s', missing expected prefix '%s' in '%s'", pFilename,
        g_pVPC->GetSourcePath(), absolutePathToOriginal);
    return false;
  }

  // supply the mirror path head
  char absolutePathToMirror[MAX_PATH];
  if (pTargetPath[0] == '\\') pTargetPath++;

  V_ComposeFileName(pMirrorPath, pTargetPath, absolutePathToMirror,
                    sizeof(absolutePathToMirror));

#ifdef _WIN32
  Sys_CreatePath(absolutePathToMirror);

  if (!CopyFile(absolutePathToOriginal, absolutePathToMirror, FALSE)) {
    g_pVPC->VPCWarning("Cannot mirror '%s' to '%s'", absolutePathToOriginal,
                       absolutePathToMirror);
    return false;
  } else {
    g_pVPC->VPCStatus(true, "Mirror: '%s' to '%s'", absolutePathToOriginal,
                      absolutePathToMirror);
  }
#endif

  return true;
}

CUtlString Sys_GuidFromFileName(const char *szFileName) {
  // set the GUID
  MD5Context_t ctx;
  unsigned char digest[MD5_DIGEST_LENGTH];
  V_memset(&ctx, 0, sizeof(ctx));
  V_memset(digest, 0, sizeof(digest));
  MD5Init(&ctx);
  MD5Update(&ctx, (unsigned char *)szFileName, strlen(szFileName));
  MD5Final(digest, &ctx);

  char szMD5[64];
  V_binarytohex(digest, MD5_DIGEST_LENGTH, szMD5, sizeof(szMD5));
  V_strupr(szMD5);

  char szGUID[MAX_PATH];
  V_snprintf(szGUID, sizeof(szGUID), "{%8.8s-%4.4s-%4.4s-%4.4s-%12.12s}", szMD5,
             &szMD5[8], &szMD5[12], &szMD5[16], &szMD5[20]);
  return szGUID;
}