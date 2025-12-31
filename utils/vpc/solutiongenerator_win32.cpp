// Copyright Valve Corporation, All rights reserved.

#include "vpc.h"
#include "dependencies.h"
#include "tier1/checksum_md5.h"

#if defined(_WIN32) && !defined(_X360)
#include "winlite.h"
#include <io.h>
#endif

#include <memory>

#include "tier0/memdbgon.h"

struct CVCProjInfo {
  CUtlString m_ProjectName;
  CUtlString m_ProjectGUID;
};

struct RegStartPoint {
  HKEY baseKey;
  const char *const baseDir;
};

class IBaseSolutionWriter_Win32 {
 public:
  virtual ~IBaseSolutionWriter_Win32() = 0;

  virtual void WriteHeader() = 0;
  virtual void WriteProjects(CUtlVector<CDependency_Project *> &projects) = 0;

 protected:
  static void ConvertToRelativePath(CVPC *vpc, char (&szFullPath)[MAX_PATH]) {
    // If file in start directory, drop latter from full path.
    if (V_strstr(szFullPath, vpc->GetStartDirectory()) == &szFullPath[0]) {
      intp relativeFilePathIdx = V_strlen(vpc->GetStartDirectory()) +
                                 sizeof(CORRECT_PATH_SEPARATOR_S) - 1;
      if (relativeFilePathIdx < sizeof(szFullPath)) {
        V_memmove(szFullPath, szFullPath + relativeFilePathIdx,
                  sizeof(szFullPath) - relativeFilePathIdx);
      }
    }
  }

  static const char *FindInFile(CVPC *vpc, const char *pFilename,
                                const char *pFileData, const char *pSearchFor) {
    const char *pPos = V_stristr(pFileData, pSearchFor);
    if (!pPos) {
      vpc->VPCError("Can't find %s in %s.", pSearchFor, pFilename);
    }

    return pPos + V_strlen(pSearchFor);
  }

  static void GetProjectInfos(CVPC *vpc,
                              CUtlVector<CDependency_Project *> &projects,
                              CUtlVector<CVCProjInfo> &vcprojInfos) {
    for (intp i = 0; i < projects.Count(); i++) {
      CDependency_Project *pCurProject = projects[i];
      const char *pFilename = pCurProject->m_ProjectFilename.String();

      CVCProjInfo vcprojInfo;

      char *pFileData;
      int nResult = Sys_LoadFile(pFilename, (void **)&pFileData, false);
      if (nResult == -1)
        vpc->VPCError("Can't open %s to get ProjectGUID.", pFilename);

      const char *pSearchFor;
      if (vpc->Is2010PlusFileFormat()) {
        pSearchFor = "<ProjectGuid>{";
      } else {
        pSearchFor = "ProjectGUID=\"{";
      }

      const char *pPos = FindInFile(vpc, pFilename, pFileData, pSearchFor);
      char szGuid[37];
      const char *pGuid = pPos;
      V_strncpy(szGuid, pGuid, sizeof(szGuid));
      vcprojInfo.m_ProjectGUID = szGuid;

      const char *pEnd;
      if (vpc->Is2010PlusFileFormat()) {
        pPos = FindInFile(vpc, pFilename, pFileData, "<ProjectName>");
        pEnd = V_stristr(pPos, "<");
      } else {
        pPos = FindInFile(vpc, pFilename, pFileData, "Name=\"");
        pEnd = V_stristr(pPos, "\"");
      }

      if (!pEnd || (pEnd - pPos) > 1024 || (pEnd - pPos) <= 0)
        vpc->VPCError("Can't find valid 'Name=' in %s.", pFilename);

      char szName[256];
      V_strncpy(szName, pPos, (pEnd - pPos) + 1);
      vcprojInfo.m_ProjectName = szName;

      vcprojInfos.AddToTail(vcprojInfo);

      free(pFileData);
    }
  }
};

IBaseSolutionWriter_Win32::~IBaseSolutionWriter_Win32() {}

class CSlnSolutionWriter_Win32 : public IBaseSolutionWriter_Win32 {
 public:
  CSlnSolutionWriter_Win32(const char *fileName, CVPC *vpc)
      : m_fileName{fileName}, m_fp{fopen(fileName, "wt")}, m_vpc{vpc} {
    if (!m_fp) {
      vpc->VPCError("Can't open %s for writing.", fileName);
    }
  }
  ~CSlnSolutionWriter_Win32() {
    if (m_fp) fclose(m_fp);
  }

  void WriteHeader() override {
    if (m_vpc->Is2026()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 12.00\n");  // still on 12
      fprintf(m_fp, "# Visual Studio 2026\n");
      fprintf(m_fp, "MinimumVisualStudioVersion = 10.0.40219.1\n");
    } else if (m_vpc->Is2022()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 12.00\n");  // still on 12
      fprintf(m_fp, "# Visual Studio 2022\n");
      fprintf(m_fp, "MinimumVisualStudioVersion = 10.0.40219.1\n");
    } else if (m_vpc->Is2015()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 12.00\n");  // still on 12
      fprintf(m_fp, "# Visual Studio 2015\n");
    } else if (m_vpc->Is2013()) {
      fprintf(
          m_fp,
          "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format Version "
          "12.00\n");  // Format didn't change from VS 2012 to VS 2013
      fprintf(m_fp, "# Visual Studio 2013\n");
    } else if (m_vpc->Is2012()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 12.00\n");
      fprintf(m_fp, "# Visual Studio 2012\n");
    } else if (m_vpc->Is2010()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 11.00\n");
      fprintf(m_fp, "# Visual Studio 2010\n");
    } else if (m_vpc->Is2008()) {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 10.00\n");
      fprintf(m_fp, "# Visual Studio 2008\n");
    } else {
      fprintf(m_fp,
              "\xef\xbb\xbf\nMicrosoft Visual Studio Solution File, Format "
              "Version 9.00\n");
      fprintf(m_fp, "# Visual Studio 2005\n");
    }

    fprintf(m_fp, "#\n");
    fprintf(m_fp, "# Automatically generated solution:\n");
    fprintf(m_fp, "# devtools\\bin\\vpc ");

    for (int k = 1; k < __argc; ++k) fprintf(m_fp, "%s ", __argv[k]);

    fprintf(m_fp, "\n");
    fprintf(m_fp, "#\n");
    fprintf(m_fp, "#\n");
  }

  void WriteProjects(CUtlVector<CDependency_Project *> &projects) override {
    char szSolutionGUID[256];
    GetVCPROJSolutionGUID(szSolutionGUID);

    CUtlVector<CVCProjInfo> vcprojInfos;
    GetProjectInfos(m_vpc, projects, vcprojInfos);

    for (intp i = 0; i < projects.Count(); i++) {
      CDependency_Project *pCurProject = projects[i];
      CVCProjInfo *pProjInfo = &vcprojInfos[i];

      // Get a relative filename for the vcproj file.
      const char *pFullProjectFilename =
          pCurProject->m_ProjectFilename.String();
      char szRelativeFilename[MAX_PATH];
      if (!V_MakeRelativePath(pFullProjectFilename, m_vpc->GetSourcePath(),
                              szRelativeFilename, sizeof(szRelativeFilename)))
        m_vpc->VPCError(
            "Can't make a relative path (to the base source directory) for %s.",
            pFullProjectFilename);

      fprintf(m_fp, "Project(\"%s\") = \"%s\", \"%s\", \"{%s}\"\n",
              szSolutionGUID, pProjInfo->m_ProjectName.String(),
              szRelativeFilename, pProjInfo->m_ProjectGUID.String());
      bool bHasDependencies = false;

      for (intp iTestProject = 0; iTestProject < projects.Count();
           iTestProject++) {
        if (i == iTestProject) continue;

        CDependency_Project *pTestProject = projects[iTestProject];
        if (pCurProject->DependsOn(pTestProject,
                                   k_EDependsOnFlagCheckNormalDependencies |
                                       k_EDependsOnFlagTraversePastLibs |
                                       k_EDependsOnFlagRecurse) ||
            pCurProject->DependsOn(pTestProject,
                                   k_EDependsOnFlagCheckAdditionalDependencies |
                                       k_EDependsOnFlagTraversePastLibs)) {
          if (!bHasDependencies) {
            fprintf(m_fp,
                    "\tProjectSection(ProjectDependencies) = postProject\n");
            bHasDependencies = true;
          }
          fprintf(m_fp, "\t\t{%s} = {%s}\n",
                  vcprojInfos[iTestProject].m_ProjectGUID.String(),
                  vcprojInfos[iTestProject].m_ProjectGUID.String());
        }
      }
      if (bHasDependencies) fprintf(m_fp, "\tEndProjectSection\n");

      fprintf(m_fp, "EndProject\n");
    }

    if (!m_vpc->Is2010()) {
      // if /slnItems <filename> is passed on the command line, build a Solution
      // Items project
      const char *pSolutionItemsFilename = m_vpc->GetSolutionItemsFilename();
      if (pSolutionItemsFilename[0] != '\0') {
        fprintf(m_fp,
                "Project(\"{2150E333-8FDC-42A3-9474-1A3956D46DE8}\") = "
                "\"Solution Items\", \"Solution Items\", "
                "\"{AAAAAAAA-8B4A-11D0-8D11-90A07D6D6F7D}\"\n");
        fprintf(m_fp, "\tProjectSection(SolutionItems) = preProject\n");

        WriteSolutionItems();

        fprintf(m_fp, "\tEndProjectSection\n");
        fprintf(m_fp, "EndProject\n");
      }
    }

    // Write solution global data
    WriteGlobalSolutionData(vcprojInfos);
  }

 private:
  const char *m_fileName;
  FILE *m_fp;
  CVPC *m_vpc;

  // Parse g_SolutionItemsFilename, reading in filenames (including wildcards),
  // and add them to the Solution Items project we're already writing.
  void WriteSolutionItems() {
    char szFullSolutionItemsPath[MAX_PATH];
    if (V_IsAbsolutePath(m_vpc->GetSolutionItemsFilename()))
      V_strncpy(szFullSolutionItemsPath, m_vpc->GetSolutionItemsFilename(),
                sizeof(szFullSolutionItemsPath));
    else
      V_ComposeFileName(
          m_vpc->GetStartDirectory(), m_vpc->GetSolutionItemsFilename(),
          szFullSolutionItemsPath, sizeof(szFullSolutionItemsPath));

    m_vpc->GetScript().PushScript(szFullSolutionItemsPath);

    int numSolutionItems = 0;
    while (m_vpc->GetScript().GetData()) {
      // read a line
      const char *pToken = m_vpc->GetScript().GetToken(false);

      // strip out \r\n chars
      char *end = V_strstr(pToken, "\n");
      if (end) {
        *end = '\0';
      }
      end = V_strstr(pToken, "\r");
      if (end) {
        *end = '\0';
      }

      // bail on strings too small to be paths
      if (V_strlen(pToken) < 3) continue;

      // compose an absolute path w/o any ../
      char szFullPath[MAX_PATH];
      if (V_IsAbsolutePath(pToken))
        V_strncpy(szFullPath, pToken, sizeof(szFullPath));
      else
        V_ComposeFileName(m_vpc->GetStartDirectory(), pToken, szFullPath,
                          sizeof(szFullPath));

      if (!V_RemoveDotSlashes(szFullPath)) continue;

      if (V_strstr(szFullPath, "*") != NULL) {
        // wildcard!
        char szWildcardPath[MAX_PATH];
        V_strncpy(szWildcardPath, szFullPath, sizeof(szWildcardPath));
        V_StripFilename(szWildcardPath);

        struct _finddata32_t data;
        intptr_t handle = _findfirst32(szFullPath, &data);
        if (handle != -1L) {
          do {
            if ((data.attrib & _A_SUBDIR) == 0) {
              // not a dir, just a filename - add it
              V_ComposeFileName(szWildcardPath, data.name, szFullPath,
                                sizeof(szFullPath));

              if (V_RemoveDotSlashes(szFullPath)) {
                ConvertToRelativePath(m_vpc, szFullPath);

                fprintf(m_fp, "\t\t%s = %s\n", szFullPath, szFullPath);

                ++numSolutionItems;
              }
            }
          } while (_findnext32(handle, &data) == 0);

          _findclose(handle);
        }
      } else {
        ConvertToRelativePath(m_vpc, szFullPath);

        // just a file - add it
        fprintf(m_fp, "    <File Path=\"%s\" />\n", szFullPath);

        ++numSolutionItems;
      }
    }

    m_vpc->GetScript().PopScript();

    Msg("Found %d solution files in %s\n", numSolutionItems,
        m_vpc->GetSolutionItemsFilename());
  }

  void WriteGlobalSolutionData(const CUtlVector<CVCProjInfo> &vcprojInfos) {
    fprintf(m_fp, "Global\n");

    {
      // Write solution configuration platforms
      fprintf(
          m_fp,
          "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
      WriteSolutionConfigurationPlatforms();
      fprintf(m_fp, "\tEndGlobalSection\n");
    }

    {
      // Write project configuration platforms.
      fprintf(
          m_fp,
          "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
      WriteProjectConfigurationPlatforms(vcprojInfos);
      fprintf(m_fp, "\tEndGlobalSection\n");
    }

    {
      // Do not hide solution node
      fprintf(m_fp, "\tGlobalSection(SolutionProperties) = preSolution\n");
      fprintf(m_fp, "\t\tHideSolutionNode = FALSE\n");
      fprintf(m_fp, "\tEndGlobalSection\n");
    }

    {
      // Set solution GUID for extensions
      fprintf(m_fp, "\tGlobalSection(ExtensibilityGlobals) = postSolution\n");
      fprintf(m_fp, "\t\tSolutionGuid = %s\n",
              Sys_GuidFromFileName(m_fileName).Get());
      fprintf(m_fp, "\tEndGlobalSection\n");
    }

    fprintf(m_fp, "EndGlobal\n");
  }

  void WriteSolutionConfigurationPlatforms() {
    const char *pSolutionTargetPlatformName =
        m_vpc->IsPlatformDefined("win64") ? "x64" : "x86";

    CUtlVector<CUtlString> configurationNames;
    m_vpc->GetProjectGenerator()->GetAllConfigurationNames(configurationNames);

    for (auto &&configuration : configurationNames) {
      fprintf(m_fp, "\t\t%s|%s = %s|%s\n", configuration.Get(),
              pSolutionTargetPlatformName, configuration.Get(),
              pSolutionTargetPlatformName);
    }
  }

  void WriteProjectConfigurationPlatforms(
      const CUtlVector<CVCProjInfo> &vcprojInfos) {
    const char *pSolutionTargetPlatformName =
        m_vpc->IsPlatformDefined("win64") ? "x64" : "x86";
    const char *pProjectTargetPlatformName =
        m_vpc->IsPlatformDefined("win64") ? "x64" : "Win32";

    CUtlVector<CUtlString> configurationNames;
    m_vpc->GetProjectGenerator()->GetAllConfigurationNames(configurationNames);

    for (const CVCProjInfo &projInfo : vcprojInfos) {
      for (auto &&configuration : configurationNames) {
        fprintf(m_fp, "\t\t{%s}.%s|%s.ActiveCfg = %s|%s\n",
                projInfo.m_ProjectGUID.Get(), configuration.Get(),
                pSolutionTargetPlatformName, configuration.Get(),
                pProjectTargetPlatformName);
        fprintf(m_fp, "\t\t{%s}.%s|%s.Build.0 = %s|%s\n",
                projInfo.m_ProjectGUID.Get(), configuration.Get(),
                pSolutionTargetPlatformName, configuration.Get(),
                pProjectTargetPlatformName);
      }
    }
  }

  void GetVCPROJSolutionGUID(char (&szSolutionGUID)[256]) {
    if (m_vpc->Is2026() || m_vpc->Is2022()) {
      V_strncpy(szSolutionGUID, "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}",
                ARRAYSIZE(szSolutionGUID));
      return;
    }

    int firstVer;
    const int lastVer = 14;  // Handle up to VS 14, AKA VS 2015

    // VS2010+ uses 10 as first version.
    if (m_vpc->Is2010PlusFileFormat()) {
      firstVer = 10;
    } else if (m_vpc->Is2008()) {
      firstVer = 9;
    } else {
      firstVer = 8;
    }

    for (int vsVer = firstVer; vsVer <= lastVer; ++vsVer) {
      // Handle both VisualStudio and VCExpress (used by some SourceSDK
      // customers)
      RegStartPoint searchPoints[]{
          {HKEY_LOCAL_MACHINE,
           // Visual Studio Professional
           "Software\\Microsoft\\VisualStudio\\%d.0\\Projects"},
          {HKEY_LOCAL_MACHINE,
           // Visual Studio Professional on x64 machine
           "Software\\WOW6432Node\\Microsoft\\VisualStudio\\%d.0\\Projects"},
          {HKEY_LOCAL_MACHINE,
           // VC Express 2010 and 2012
           "Software\\Microsoft\\VCExpress\\%d.0\\Projects"},
          {HKEY_CURRENT_USER,
           // WinDev Express -- VS Express starting with VS 2013
           "Software\\Microsoft\\WDExpress\\%d.0_Config\\Projects"},
      };

      for (const auto &searchPoint : searchPoints) {
        char pRegKeyName[1000];
        V_snprintf(pRegKeyName, ARRAYSIZE(pRegKeyName), searchPoint.baseDir,
                   vsVer);
        HKEY hKey;
        LONG ret =
            RegOpenKeyEx(searchPoint.baseKey, pRegKeyName, 0, KEY_READ, &hKey);
        if (!hKey) continue;

        for (int i = 0; i < 200; i++) {
          char szKeyName[MAX_PATH];
          DWORD dwKeyNameSize = sizeof(szKeyName);
          ret = RegEnumKeyEx(hKey, i, szKeyName, &dwKeyNameSize, NULL, NULL,
                             NULL, NULL);

          if (ret == ERROR_NO_MORE_ITEMS) break;

          HKEY hSubKey;
          ret = RegOpenKeyEx(hKey, szKeyName, 0, KEY_READ, &hSubKey);
          if (ret == ERROR_SUCCESS) {
            DWORD dwType;
            char ext[MAX_PATH];
            DWORD dwExtLen = sizeof(ext);
            ret = RegQueryValueEx(hSubKey, "DefaultProjectExtension", NULL,
                                  &dwType, (BYTE *)ext, &dwExtLen);
            RegCloseKey(hSubKey);

            // VS 2012 and beyond has the DefaultProjectExtension as vcxproj
            // instead of vcproj
            if (ret == ERROR_SUCCESS && dwType == REG_SZ &&
                (V_stricmp(ext, "vcproj") == 0 ||
                 V_stricmp(ext, "vcxproj") == 0)) {
              V_strncpy(szSolutionGUID, szKeyName, ARRAYSIZE(szSolutionGUID));
              RegCloseKey(hKey);
              return;
            }
          }
        }

        RegCloseKey(hKey);
      }
    }

    m_vpc->VPCError(
        "Unable to find RegKey for .vcproj or .vcxproj files in solutions.");
  }
};

class CSlnxSolutionWriter_Win32 : public IBaseSolutionWriter_Win32 {
 public:
  CSlnxSolutionWriter_Win32(const char *fileName, CVPC *vpc)
      : m_fp{fopen(fileName, "wt")}, m_vpc{vpc} {
    if (!m_fp) {
      vpc->VPCError("Can't open %s for writing.", fileName);
    }
  }
  ~CSlnxSolutionWriter_Win32() {
    if (m_fp) fclose(m_fp);
  }

  void WriteHeader() override {
    fprintf(m_fp, "<!-- Automatically generated solution: -->\n");
    fprintf(m_fp, "<!--  devtools\\bin\\vpc ");

    for (int k = 1; k < __argc; ++k) fprintf(m_fp, "%s ", __argv[k]);

    fprintf(m_fp, "-->\n");
    fprintf(m_fp, "<Solution>\n");
  }

  void WriteProjects(CUtlVector<CDependency_Project *> &projects) override {
    CUtlVector<CVCProjInfo> vcprojInfos;
    GetProjectInfos(m_vpc, projects, vcprojInfos);

    for (intp i = 0; i < projects.Count(); i++) {
      CDependency_Project *pCurProject = projects[i];

      // Get a relative filename for the vcproj file.
      const char *pFullProjectFilename =
          pCurProject->m_ProjectFilename.String();
      char szRelativeFilename[MAX_PATH];
      if (!V_MakeRelativePath(pFullProjectFilename, m_vpc->GetSourcePath(),
                              szRelativeFilename, sizeof(szRelativeFilename)))
        m_vpc->VPCError(
            "Can't make a relative path (to the base source directory) for %s.",
            pFullProjectFilename);

      fprintf(m_fp, "  <Project Path=\"%s\" />\n", szRelativeFilename );
    }

    if (!m_vpc->Is2010()) {
      // if /slnItems <filename> is passed on the command line, build a Solution
      // Items project
      const char *pSolutionItemsFilename = m_vpc->GetSolutionItemsFilename();
      if (pSolutionItemsFilename[0] != '\0') {
        fprintf(m_fp, "  <Folder Name=\"/Solution Items/\">\n");

        WriteSolutionItems();

        fprintf(m_fp, "  </Folder>\n");
      }
    }

    const char *pSolutionTargetPlatformName =
        m_vpc->IsPlatformDefined("win64") ? "x64" : "x86";

    fprintf(m_fp, "  <Configurations>\n");
    fprintf(m_fp, "    <Platform Name=\"%s\" />\n",
            pSolutionTargetPlatformName);
    fprintf(m_fp, "  </Configurations>\n");

    fprintf(m_fp, "</Solution>\n");
  }

 private:
  FILE *m_fp;
  CVPC *m_vpc;

  // Parse g_SolutionItemsFilename, reading in filenames (including wildcards),
  // and add them to the Solution Items project we're already writing.
  void WriteSolutionItems() {
    char szFullSolutionItemsPath[MAX_PATH];
    if (V_IsAbsolutePath(m_vpc->GetSolutionItemsFilename()))
      V_strncpy(szFullSolutionItemsPath, m_vpc->GetSolutionItemsFilename(),
                sizeof(szFullSolutionItemsPath));
    else
      V_ComposeFileName(
          m_vpc->GetStartDirectory(), m_vpc->GetSolutionItemsFilename(),
          szFullSolutionItemsPath, sizeof(szFullSolutionItemsPath));

    m_vpc->GetScript().PushScript(szFullSolutionItemsPath);

    int numSolutionItems = 0;
    while (m_vpc->GetScript().GetData()) {
      // read a line
      const char *pToken = m_vpc->GetScript().GetToken(false);

      // strip out \r\n chars
      char *end = V_strstr(pToken, "\n");
      if (end) {
        *end = '\0';
      }
      end = V_strstr(pToken, "\r");
      if (end) {
        *end = '\0';
      }

      // bail on strings too small to be paths
      if (V_strlen(pToken) < 3) continue;

      // compose an absolute path w/o any ../
      char szFullPath[MAX_PATH];
      if (V_IsAbsolutePath(pToken))
        V_strncpy(szFullPath, pToken, sizeof(szFullPath));
      else
        V_ComposeFileName(m_vpc->GetStartDirectory(), pToken, szFullPath,
                          sizeof(szFullPath));

      if (!V_RemoveDotSlashes(szFullPath)) continue;

      if (V_strstr(szFullPath, "*") != NULL) {
        // wildcard!
        char szWildcardPath[MAX_PATH];
        V_strncpy(szWildcardPath, szFullPath, sizeof(szWildcardPath));
        V_StripFilename(szWildcardPath);

        _finddata32_t data;
        intptr_t handle = _findfirst32(szFullPath, &data);
        if (handle != -1L) {
          do {
            if ((data.attrib & _A_SUBDIR) == 0) {
              // not a dir, just a filename - add it
              V_ComposeFileName(szWildcardPath, data.name, szFullPath,
                                sizeof(szFullPath));

              if (V_RemoveDotSlashes(szFullPath)) {
                ConvertToRelativePath(m_vpc, szFullPath);

                fprintf(m_fp, "    <File Path=\"%s\" />\n", szFullPath);

                ++numSolutionItems;
              }
            }
          } while (_findnext32(handle, &data) == 0);

          _findclose(handle);
        }
      } else {
        ConvertToRelativePath(m_vpc, szFullPath);

        // just a file - add it
        fprintf(m_fp, "    <File Path=\"%s\" />\n", szFullPath);

        ++numSolutionItems;
      }
    }

    m_vpc->GetScript().PopScript();

    Msg("Found %d solution files in %s\n", numSolutionItems,
        m_vpc->GetSolutionItemsFilename());
  }
};

class CSolutionGenerator_Win32 : public IBaseSolutionGenerator {
 public:
  virtual void GenerateSolutionFile(
      const char *pSolutionFilename,
      CUtlVector<CDependency_Project *> &projects) {
    // Default extension.
    char szTmpSolutionFilename[MAX_PATH];
    if (!V_GetFileExtension(pSolutionFilename)) {
      // vs2026 defaults to slnx format.
      V_snprintf(szTmpSolutionFilename, sizeof(szTmpSolutionFilename),
                 g_pVPC->Is2026() ? "%s.slnx" : "%s.sln", pSolutionFilename);
      pSolutionFilename = szTmpSolutionFilename;
    }

    Msg("\nWriting solution file %s.\n\n", pSolutionFilename);

    {
      const char *slnExtension = V_GetFileExtension(pSolutionFilename);
      std::unique_ptr<IBaseSolutionWriter_Win32> solutionWriter =
          V_stricmp(slnExtension, "slnx") == 0
              ? std::unique_ptr<IBaseSolutionWriter_Win32>(
                    new CSlnxSolutionWriter_Win32(pSolutionFilename, g_pVPC))
              : std::unique_ptr<IBaseSolutionWriter_Win32>(
                    new CSlnSolutionWriter_Win32(pSolutionFilename, g_pVPC));

      solutionWriter->WriteHeader();
      solutionWriter->WriteProjects(projects);
    }

    Sys_CopyToMirror(pSolutionFilename);
  }
};

static CSolutionGenerator_Win32 g_SolutionGenerator_Win32;
IBaseSolutionGenerator *GetSolutionGenerator_Win32() {
  return &g_SolutionGenerator_Win32;
}
