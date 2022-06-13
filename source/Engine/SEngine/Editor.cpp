#include "Editor.h"
#include "Resources.h"
#include <set>
#include <iostream>
#include "ryml.hpp"
#include "Scene.h"
#include "SceneManager.h"
#include "DbgVars.h"
#include "Input.h"
#include "sdl.h"
#include "shlwapi.h"
#include "config.h"
#include "physfs.h"
#include <codecvt>


DBG_VAR_BOOL(dbg_isEditMode, "EditMode", false)
DBG_VAR_TRIGGER(dbg_makeBuild, "MakeBuild");

REGISTER_SYSTEM(Editor);
static bool MakeBuild();

void Editor::SetDirty(std::shared_ptr<Object> dirtyObj)
{
	if (dirtyObj != nullptr) {
		Get()->dirtyObjs.insert(dirtyObj);
		return;
	}
	LogError("Trying to SetDirty nullptr");
}


bool Editor::Init() {
	onBeforeDatabaseUnloadedHandle = AssetDatabase::Get()->onBeforeUnloaded.Subscribe([this]() {
		if (autoSaveOnTerm) {
			SaveAllDirty();
		}});
	return true;
}


void Editor::Term() {
	AssetDatabase::Get()->onBeforeUnloaded.Unsubscribe(onBeforeDatabaseUnloadedHandle);
	if (autoSaveOnTerm) {
		SaveAllDirty();
	}
}

void Editor::Update() {
	if (!CfgGetBool("godMode")) {
		return;
	}
	if (Input::GetKeyDown(SDL_SCANCODE_F4)) {
		dbg_isEditMode = !dbg_isEditMode;
	}
	auto scene = Scene::Get();
	if (dbg_isEditMode && scene && !scene->IsInEditMode()) {
		SceneManager::LoadSceneForEditing(SceneManager::GetCurrentScenePath());
	}
	else if (!dbg_isEditMode && scene && scene->IsInEditMode()) {
		SceneManager::LoadScene(SceneManager::GetCurrentScenePath());
	}
	bool save = autoSaveEveryFrame;

	if (Input::GetKey(SDL_Scancode::SDL_SCANCODE_LCTRL) && Input::GetKeyDown(SDL_SCANCODE_S)) {
		save = true;
	}

	if (save) {
		SaveAllDirty();
	}
	if (dbg_makeBuild) {
		ASSERT(MakeBuild());
	}
}

//TODO what is this function even mean?
bool Editor::IsInEditMode() const {
	auto scene = Scene::Get();
	if (!scene) {
		return false;
	}
	else {
		return scene->IsInEditMode();
	}
}

bool Editor::HasUnsavedFiles() const { return !dirtyObjs.empty(); }


void Editor::SaveAllDirty() {
	for (const auto& obj : dirtyObjs) {
		ASSERT(obj != nullptr);

		auto path = AssetDatabase::Get()->GetAssetPath(obj);

		if (path.empty()) {
			LogError("no path exist for asset '%s'", obj->GetDbgName().c_str());
			continue;
		}
		if (AssetDatabase::Get()->GetOriginalSerializedAsset(path) == nullptr) {
			LogError("no original serialized asset for '%s' in path '%s'", obj->GetDbgName().c_str(), path.c_str());
			continue;
		}

		//TODO no hardcode
		//TODO do it through asset database
		auto fullPath = AssetDatabase::Get()->GetRealPath(path);
		std::ofstream output(fullPath);
		if (!output.is_open()) {
			LogError("Failed to open '%s' for saving", fullPath.c_str());
			continue;
		}
		auto tree = AssetDatabase::Get()->GetOriginalSerializedAsset(path);
		output << tree->rootref();
	}
	dirtyObjs.clear();
}


static bool Archive(std::string path, std::string archive, int compressionLevel, std::string filter = "*") {
	DWORD sZLocSize = 1024;
	char sZLoc[1024];
	memset(sZLoc, 0, 1024);

	//TODO less hardcode
	std::string sZApp = "engine\\tools\\7z.exe";

	std::string params = "";
	params += " a -r -tzip"; // cmd
	params += " -mx=" + std::to_string(compressionLevel);
	params += " " + archive; //target
	params += " ./" + path + "/" + filter; // source

	//TODO move to platform independent stuff
	STARTUPINFOA si;
	memset(&si, 0, sizeof(STARTUPINFOA));
	si.cb = sizeof(STARTUPINFOA);

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));


	params = sZApp + " " + params;
	std::vector<char> paramsBuffer(params.begin(), params.end());
	paramsBuffer.push_back(0);

	sZApp += params;

	auto result = CreateProcessA(
		NULL
		, &paramsBuffer[0]
		, NULL
		, NULL
		, false
		, 0
		, NULL
		, NULL
		, &si
		, &pi);

	if (!result)
	{
		LogError("Failed to open 7z for build");
		return false;
	}
	else
	{
		// Successfully created the process.  Wait for it to finish.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close the handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return true;
}
static int silently_remove_directory(std::string dir) // Fully qualified name of the directory being   deleted,   without trailing backslash
{
	int len = dir.length() + 2; // required to set 2 nulls at end of argument to SHFileOperation.
	char* tempdir = (char*)malloc(len);
	memset(tempdir, 0, len);
	strcpy(tempdir, dir.c_str());

	SHFILEOPSTRUCTA file_op = {
	  NULL,
	  FO_DELETE,
	  tempdir,
	  NULL,
	  FOF_NOCONFIRMATION |
	  FOF_NOERRORUI |
	  FOF_SILENT,
	  false,
	  0,
	  "" };
	int ret = SHFileOperationA(&file_op);
	free(tempdir);
	return ret; // returns 0 on success, non zero on failure.
}
static bool MakeBuild() {
	//TODO ensure deleted
	DeleteFileA("build.zip");
	//TODO ensure deleted
	silently_remove_directory("build");
	if (!PHYSFS_mkdir("build")) {
		ASSERT_FAILED("Failed to make build folder");
		return false;
	}
	std::set<std::string> libs;
	std::set<std::string> assets;

	auto nodeMountAssets = CfgGetNode("mountAssets"); //TODO some other place for dll's to state their mount dirs
	//TODO order
	for (const auto& dir : nodeMountAssets) {
		auto dirstr = dir.as<std::string>();
		if (dirstr.find(".zip") != -1) {
			std::string out_file = dirstr;
			if (!CopyFileA(dirstr.c_str(), (std::string("build/") + dirstr).c_str(), false)) {
				ASSERT_FAILED("Failed to copy '%s' to '%s'", dirstr.c_str(), (std::string("build/") + dirstr).c_str());
				return false;
			}
			assets.insert(out_file);
		}
		else {
			if (!Archive(dirstr, "build/assets.zip", 0, "*.asset")) {
				ASSERT_FAILED("Failed to archive assets");
				return false;
			}
			assets.insert("assets.zip");
		}
	}


	auto nodeMountLibraries = CfgGetNode("mountLibraries"); //TODO some other place for dll's to state their mount dirs
	//TODO order
	for (const auto& dir : nodeMountLibraries) {
		auto dirstr = dir.as<std::string>();
		if (dirstr.find(".zip") != -1) {
			std::string out_file = dirstr;
			if (!CopyFileA(dirstr.c_str(), (std::string("build/") + out_file).c_str(), false)) {
				ASSERT_FAILED("Failed to copy '%s' to '%s'", dirstr.c_str(), (std::string("build/") + out_file).c_str());
				return false;
			}
			libs.insert(out_file);
		}
		else {
			if (!Archive(dirstr, "build/library.zip", 0)) {
				ASSERT_FAILED("Failed to archive library");
				return false;
			}
			libs.insert("library.zip");
		}
	}



	auto cfg = YAML::LoadFile("config.yaml");
	cfg["mountAssets"] = YAML::Node(YAML::NodeType::Sequence);
	cfg["mountLibraries"] = YAML::Node(YAML::NodeType::Sequence);
	cfg["assetConvertionEnabled"] = false;
	cfg["godMode"] = false;
	cfg["showFps"] = false;

	for (auto lib : libs) {
		cfg["mountLibraries"].push_back(lib);
	}
	for (auto asset : assets) {
		cfg["mountAssets"].push_back(asset);
	}
	{
		std::ofstream fout("build/config.yaml");
		fout << cfg;

		if (!CopyFileA("settings.yaml", ("build/settings.yaml"), false)) {
			ASSERT_FAILED("Failed to copy settings");
			return false;
		}
	}

	WCHAR exePath[MAX_PATH];
	GetModuleFileNameW(NULL, exePath, MAX_PATH);

	//TODO
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string fullExePath = converter.to_bytes(exePath);

	//std::string buildType = "Retail";
	int binFolderLength = fullExePath.find_last_of('\\');
	std::string binFolder = fullExePath.substr(0, binFolderLength) + "\\";
	std::string binName = fullExePath.substr(binFolderLength + 1, fullExePath.length() - binFolderLength - 1);

	std::vector<std::string> binsToCopy;
	binsToCopy.push_back(binName);
	binsToCopy.push_back("SDL2.dll");//TODO not always
	auto nodeDlls = CfgGetNode("libraries"); //TODO some other place for dll's to state their mount dirs
	for (const auto& dll : nodeDlls) {
		binsToCopy.push_back(dll.as<std::string>());
	}
	for (auto bin : binsToCopy) {
		if (!CopyFileA((binFolder + bin).c_str(), (std::string("build/") + bin).c_str(), false)) {
			ASSERT_FAILED("Failed to copy '%s' to '%s'", (binFolder + bin).c_str(), (std::string("build/") + bin).c_str());
			return false;
		}
	}
	if (!Archive("build", "build.zip", 9)) {
		ASSERT_FAILED("Failed to archive build");
		return false;
	}

	return true;
}


