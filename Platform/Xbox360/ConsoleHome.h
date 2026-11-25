#pragma once
#include "GameScanner.h"
#include "HomeCanvas.h"
#include "CloudTv.h"
#include "CollectionStore.h"
#include "ContentCatalog.h"
#include "FileWorkspace.h"
#include "StorageInfo.h"
#include "NetworkInfo.h"
#include "TrainerCatalog.h"
#include "StoreCatalog.h"
#include "ResidentPlugins.h"
#include "FtpServer.h"
#include <map>
#include <set>

class ConsoleHome {
public:
    enum Key { Left=MenuLeft, Right=MenuRight, Up=MenuUp, Down=MenuDown, Accept=MenuAccept, Back=MenuBack,
               Favorite=MenuFavorite, Refresh=MenuRefresh, PageUp=MenuPageUp, PageDown=MenuPageDown, Guide=MenuGuide, Search=MenuSearch };
    ConsoleHome(const std::string& dataDir, const std::vector<std::string>& gameRoots);
    void Load();
    std::string Press(Key key);  // Only confirmation on an installed title supplies a launch path.
    void Draw(HomeCanvas& canvas, const std::string& clock, int64_t now=-1) const;
    std::string TakeMovie();
    bool Tick();
    void StopFtp() {ftp_.Stop();}
    bool TakeNativeStoreRequest(StoreItem& item);
    bool TakeStoreInstallRequest(StoreItem& item);
    bool TakeDiscInstallRequest(std::string& mount,XboxExecutionInfo& identity);
    bool TakeDiscSystemExit();
    void DiscInstallResult(const std::string& message,bool success);
    void StoreInstallResult(const std::string& message,bool success);
    void NativeStoreResult(const std::string& message,bool success);
    bool TransferRunning() const { return transfer_.Status()==FileTransfer::Running || !copyQueue_.empty(); }
    void PlayerMessage(const std::string& text) { tv_.PlayerMessage(text); }
    void LaunchFailed();
private:
    enum Section { Library, Iptv, Apps, Emulators, Files, System, Recent, Favorites };
    enum Page { Home, Listing, Details };
    std::string dir_, notice_;
    std::vector<std::string> roots_, baseRoots_, history_, favorites_, pinned_, hidden_, launched_, folders_;
    std::vector<Game> games_, tools_;
    std::set<unsigned> translationReferences_;
    std::vector<int> toolKinds_; // 1 = installed, 2 = plugin, 3 = homebrew
    std::vector<const Game*> visible_;
    std::vector<FolderEntry> files_;
    CloudTv tv_;
    CollectionStore collections_;
    FileWorkspace fileWorkspace_;
    FtpServer ftp_;
    void ToggleFtp(bool allowUpload=false);
    FileTransfer transfer_;
    std::vector<WorkspaceCopyStep> copyQueue_;
    std::string activeCopyTarget_;
    bool StartNextCopy();
    bool copyPaused_;
    bool EnqueueCopy();
    void MovePin(int direction);
    void DrawTransfers(HomeCanvas& c) const;
    bool storeDownloads_;
    int storeDownloadRow_;
    void DrawStoreDownloads(HomeCanvas& c) const;
    void RebuildStoreDownloads();
    std::vector<size_t> storeDownloadItems_;
    void RecordEvent(const std::string& message);
    void LoadEvents();
    std::vector<std::string> events_;
    void DrawEvents(HomeCanvas& c) const;
    std::vector<StorageVolume> storageVolumes_;
    size_t storagePage_;
    void RefreshStorage();
    void DrawStorage(HomeCanvas& c) const;
    void DrawDiscInstall(HomeCanvas& c) const;
    int discDestination_=0,discMode_=0;
    bool discConfirm_=false,discPending_=false,discSystemExit_=false,discKnown_=false;
    XboxExecutionInfo discIdentity_{};
    StorageVolume discVolume_{};
    void RefreshDiscInstall();
    NetworkSnapshot network_;
    void RefreshNetwork();
    void RefreshResidentPlugins();
    void DrawResidentPlugins(HomeCanvas& c) const;
    std::vector<ResidentPluginSlot> residentPlugins_;
    std::string residentConfig_;
    void DrawNetwork(HomeCanvas& c) const;
    size_t fileRoot_;
    bool pasteConfirm_, scanPathConfirm_, fileAction_, deleteConfirm_, cutMode_;
    int fileActionRow_;
    bool multiSelect_;
    std::vector<std::string> copyNames_;
    std::string actionDir_, actionName_;
    void DrawFileActions(HomeCanvas& c) const;
    void ToggleScanPath();
    void LoadScanPaths();
    std::string copyDir_, copyName_, renameName_;
    void StartFileEdit(bool rename);
    void ConfirmFileEdit();
    int focus_, row_, subtab_, systemTab_, gameSort_, searchCursor_;
    std::string homeOrder_, homeDraft_;
    int homeEditSlot_;
    bool homeEditing_;
    void DrawHomeEditor(HomeCanvas& c) const;
    bool storeActive_, storeDetails_, storeConfirm_;
    int storeTab_, storeRow_;
    StoreCatalog store_;
    std::vector<size_t> storeVisible_;
    void RebuildStore();
    bool TranslationMatches(const StoreItem& item) const;
    bool storeTranslationMatch_;
    bool nativeStorePending_;
    bool storeInstallPending_=false;
    bool storeManage_=false,storeManageConfirm_=false;
    int storeManageRow_=0;
    void DrawStoreManage(HomeCanvas& c,const StoreItem& item) const;
    StoreItem storeInstallItem_;
    bool nativeBatch_=false;
    bool StartNativeStore(const StoreItem& item);
    bool NextNativeStore();
    StoreItem nativeStoreItem_;
    void DrawStore(HomeCanvas& c) const;
    bool importPreview_;
    std::vector<std::string> importFavorites_, importHistory_;
    void PreviewAuroraProfile();
    void ApplyAuroraProfile();
    void DrawAuroraProfile(HomeCanvas& c) const;
    int collectionIndex_, collectionRow_, keyboardMode_; // 0 search, 1 create, 2 rename
    bool searchActive_, collectionPicker_, deleteCollectionConfirm_;
    std::string searchTerm_, searchDraft_, pickerGame_;
    void ApplySearch();
    bool contentActive_, contentKnown_;
    int contentTab_, contentRow_;
    XboxExecutionInfo contentInfo_;
    XboxTitleMetadata detailMetadata_;
    void RefreshTitleMetadata();
    mutable std::map<std::string,std::string> artFolders_;
    mutable bool artDirty_;
    std::string ArtFolder(const Game& game) const;
    std::map<std::string,std::string> artChoices_;
    bool artEditing_;
    int artEditRow_;
    std::string artEditFolder_, artEditId_, artDraft_;
    std::string ArtPath(const Game& game,bool cover) const;
    void DrawArtEditor(HomeCanvas& c) const;
    std::vector<XboxContentPackage> contentItems_;
    std::vector<TrainerEntry> trainers_;
    bool trainerConfirm_;
    void DrawTrainers(HomeCanvas& c) const;
    void OpenContent();
    size_t ContentCount() const;
    const XboxContentPackage* ContentAt(size_t index) const;
    void DrawContent(HomeCanvas& c) const;
    Section section_;
    Page page_;
    size_t Count() const;
    void Enter(Section section);
    void Rebuild();
    void Reload();
    void ScanLibrary();
    bool SaveProfile() const;
    bool Starred(const Game& game) const;
    std::vector<std::string> HomeShortcuts() const;
    const Game* Installed(const std::string& path) const;
    void DrawHome(HomeCanvas& c) const;
    void DrawSection(HomeCanvas& c) const;
};
