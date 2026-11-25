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
#include "UnityCatalog.h"
#include "ContentBackup.h"
#include "ContentInstall.h"
#include "PackageImport.h"
#include "DiscSet.h"
#include "DiscInstaller.h"
#include "DashboardUpdate.h"
#include "ConsoleStatus.h"
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
    bool TakeProviderRequest(StoreProvider& provider);
    bool TakeUnityRequest(XboxExecutionInfo& info);
    bool TakeArtRequest(unsigned& title,bool& cover,bool& remote);
    bool TakeCoverBatch(std::vector<unsigned>& titles);
    bool TakeDashboardUpdate(StoreItem& item);
    void DashboardUpdateResult(const std::string& message,bool success);
    void CoverBatchResult(unsigned saved,unsigned failed,bool cancelled);
    void ArtResult(const std::string& message,bool success);
    bool TakeBackupRequest(XboxContentPackage& selected);
    bool TakeContentInstall(ContentInstall::Request& item,XboxExecutionInfo& game);
    void ContentInstallResult(const std::string& message,bool success);
    bool TakeRecoverRequest(ContentBackup::Record& selected);
    void RecoverResult(const std::string& message,bool success);
    void BackupResult(const std::string& message,bool success);
    bool TakeUnityDownload(XboxExecutionInfo& info,UnityUpdate& update);
    void UnityDownloadResult(const std::string& message,bool success);
    void UnityResult(const std::vector<UnityUpdate>& updates,const std::string& message,bool success);
    void ProviderResult(const std::string& message,bool success);
    bool TakeStoreInstallRequest(StoreItem& item);
    bool TakeDiscInstallRequest(DiscInstallRequest& request);
    bool TakePackageImport(PackageLaunch::Ticket& ticket);
    void PackageImportResult(const std::string& message,bool success);
    bool TakeDiscSystemExit();
    void DiscInstallResult(const std::string& message,bool success);
    void StoreInstallResult(const std::string& message,bool success);
    void NativeStoreResult(const std::string& message,bool success);
    bool TransferRunning() const { return transfer_.Status()==FileTransfer::Running || !copyQueue_.empty(); }
    void PlayerMessage(const std::string& text) { tv_.PlayerMessage(text); }
    void LaunchFailed();
    void RecordLaunchAttempt(const std::string& path);
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
    bool packageImportPending_=false;
    PackageLaunch::Ticket packageImportTicket_{};
    enum DiscMethod { ExtractDvd, ConvertGod, ImportPackage, OpenSystem, DiscMethodCount };
    int discDestination_=0,discMode_=ExtractDvd;
    DiscInstallRequest pendingDisc_;
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
    bool storeProviders_=false,providerPending_=false;
    int providerRow_=0;
    StoreProvider providerRequest_;
    void DrawProviders(HomeCanvas& c) const;
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
    bool backupConfirm_=false,backupPending_=false,recoverConfirm_=false,recoverPending_=false,archiveTuConfirm_=false;
    ContentBackup::Record recoverItem_;
    bool contentInstallActive_=false,contentInstallConfirm_=false,contentInstallPending_=false,contentInstallKnown_=false;
    ContentInstall::Request contentInstallItem_{};
    std::string contentInstallStaged_,contentInstallBackup_;
    void RefreshContentInstall();
    void DrawContentInstall(HomeCanvas& c) const;
    std::vector<ContentBackup::Record> backups_;
    bool pruneQueueConfirm_=false;
    XboxContentPackage backupItem_{};
    std::vector<XboxContentPackage> stagedItems_,archivedItems_,disabledItems_;
    bool toggleTuConfirm_=false;
    bool restoreArchivedConfirm_=false;
    bool artPending_=false,artRemote_=false,artCover_=true;
    bool coverBatchConfirm_=false,coverBatchPending_=false;
    bool updateActive_=false,updateConfirm_=false,updatePending_=false,updateKnown_=false;
    StoreItem updateItem_{};
    void RefreshDashboardUpdate();
    void DrawDashboardUpdate(HomeCanvas& c) const;
    std::vector<unsigned> coverBatch_;
    bool discSetActive_=false;
    size_t discSetRow_=0;
    XboxExecutionInfo discSetIdentity_{};
    std::vector<DiscSet::Entry> discSet_;
    void DrawDiscSet(HomeCanvas& c) const;
    void PrepareCoverBatch();
    unsigned artTitle_=0;
    ConsoleStatus status_;
    unsigned theme_=0;
    bool sessionExitConfirm_=false;
    void DrawConsoleStatus(HomeCanvas& c) const;
    bool unityActive_=false,unityPending_=false,unityDownloadPending_=false,unityConfirm_=false;
    UnityUpdate unityDownload_;
    int unityRow_=0;
    std::vector<UnityUpdate> unityUpdates_;
    void DrawUnity(HomeCanvas& c) const;
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
