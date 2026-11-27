#pragma once
#include "GameScanner.h"
#include "HomeCanvas.h"
#include "CloudTv.h"
#include "Modules/Emulators/Ps1CoverIndex.h"
#include "CollectionStore.h"
#include "ContentCatalog.h"
#include "FileWorkspace.h"
#include "StorageInfo.h"
#include "TransferRate.h"
#include "NetworkInfo.h"
#include "LanPeerProbe.h"
#include "LanPeerConfig.h"
#include "SmbProbe.h"
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
#include "SmbReadSession.h"
#include "SmbConsoleStage.h"
#endif
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
#include "ClockWeather.h"
#include "LaunchStatistics.h"
#include "GpdAchievements.h"
#include "NotificationCenter.h"
#include "QuickGuide.h"
#include "SocialProfile.h"
#include "ProfilePrivileges.h"
#include "AvatarPreview.h"
#include "AvatarManifest.h"
#include "FriendRoster.h"
#include "SystemTelemetry.h"
#include "VideoTelemetry.h"
#include "DisplaySafeArea.h"
#include "IdleScreen.h"
#include "ResidentPlugins.h"
#include "ThermalTelemetry.h"
#include "ConsolePower.h"
#include "PluginRegistry.h"
#include "FtpServer.h"
#include <map>
#include <utility>
#include <set>

class ConsoleHome {
public:
    enum Key { Left=MenuLeft, Right=MenuRight, Up=MenuUp, Down=MenuDown, Accept=MenuAccept, Back=MenuBack,
               Favorite=MenuFavorite, Refresh=MenuRefresh, PageUp=MenuPageUp, PageDown=MenuPageDown, Guide=MenuGuide, Search=MenuSearch };
    ConsoleHome(const std::string& dataDir, const std::vector<std::string>& gameRoots, unsigned initialLayout=1);
    void Load();
    unsigned SafeArea()const{return safeArea_;}
    unsigned IdleMinutes()const{return idleMinutes_;}
    unsigned Language()const{return language_;}
    std::string ClockLabel(int64_t epoch)const{return ClockWeather::Time(epoch,clockSettings_.offset);}
    bool TakeWeatherRequest(unsigned& city){if(!weatherPending_)return false;weatherPending_=false;city=clockSettings_.city;return city!=0;}
    bool WeatherAutoDue(int64_t now);
    bool ApprovedHeader()const{return page_==Home&&layout_==1&&!storeActive_&&!homeEditing_&&!importPreview_;}
    void WeatherResult(const ClockWeather::Reading& reading,const std::string& message){if(reading.valid&&reading.city==clockSettings_.city)weather_=reading;lastWeatherAttempt_=std::time(nullptr);notice_=message;}
    size_t UnreadNotifications()const{return notifications_.UnreadCount();}
    bool QuickGuideOpen()const{return quickGuideOpen_;}
    // Console provider or explicitly marked host test fixture.
    void SetSocialSnapshot(const InfinitySocial::Snapshot& snapshot,int preferred=-1){
        const auto previous=social_;
        social_=InfinitySocial::Resolve(snapshot,preferred);friendRoster_.Observe(social_);avatarPreview_.Observe(social_);privileges_.Observe(social_);
        avatarRefresh_.Observe(social_);nativeSocialQueue_.Observe(social_);if(!InfinitySocial::SameArtAccount(previous,social_)){avatarManifest_={};avatarBudget_={};avatarExitConfirm_.Cancel();avatarBodyPending_=false;privilegePending_=false;friendRosterPending_=false;}
        if(!friendRoster_.KnownFor(social_)){friendFreshness_.Clear();friendRow_=0;}
    }
    const InfinitySocial::Header& SocialHeader()const{return social_;}
    bool SocialGuideFocused()const{return quickGuideOpen_&&!quickGuideEditing_&&socialGuideFocus_;}
    bool FriendGuideOpen()const{return quickGuideOpen_&&friendGuideOpen_;}
    bool AvatarGuideOpen()const{return quickGuideOpen_&&avatarGuideOpen_;}
    bool TakeAvatarBodyRequest(){const bool pending=avatarBodyPending_;avatarBodyPending_=false;return pending&&AvatarGuideOpen();}
    bool AvatarAutoDue(std::uint64_t now)const{return avatarRefresh_.Due(social_,now);}
    void AvatarBodyResult(const InfinitySocial::AvatarPreview& preview,const std::string& message,
                          std::uint64_t now=0,bool manual=true){
        avatarPreview_=preview;avatarPreview_.Observe(social_);avatarRefresh_.Mark(social_,now);
        if(manual)notice_=message;
    }
    void AvatarManifestResult(const InfinitySocial::Header& before,const InfinitySocial::AvatarManifestSummary& result){
        avatarManifest_=InfinitySocial::SameArtAccount(before,social_)?result:InfinitySocial::AvatarManifestSummary{};
    }
    bool CommunicationGuideOpen()const{return quickGuideOpen_&&communicationGuideOpen_;}
    bool PrivilegeGuideOpen()const{return CommunicationGuideOpen()&&privilegeGuideOpen_;}
    bool TakePrivilegeRequest(){const bool requested=privilegePending_;privilegePending_=false;return requested&&PrivilegeGuideOpen();}
    void PrivilegeResult(const InfinitySocial::ProfilePrivileges& result,const std::string& message){
        privileges_=result;privileges_.Observe(social_);notice_=message;
    }
    bool ExpirePrivileges(std::uint64_t now){return privileges_.Expire(social_,now);}
    void AvatarBudgetResult(const InfinitySocial::Header& before,const InfinitySocial::AvatarAssetBudget& budget){
        avatarBudget_=InfinitySocial::SameArtAccount(before,social_)?budget:InfinitySocial::AvatarAssetBudget{};
    }
    void SetAvatarCapabilities(const InfinitySocial::AvatarCapabilities& caps){avatarCapabilities_=caps;}
    bool SetAvatarFrame(const InfinitySocial::Header& before,unsigned w,unsigned h,const std::vector<unsigned char>& rgba){
        return avatarPreview_.Set(before,social_,w,h,rgba);
    }
    bool TakeFriendRosterRequest(){const bool pending=friendRosterPending_;friendRosterPending_=false;return pending&&FriendGuideOpen();}
    void FriendRosterResult(const InfinitySocial::FriendRoster& roster,const std::string& message,std::uint64_t now=0);
    bool UpdateFriendPresence(std::uint64_t now,bool autoEnabled,bool allowed);
    bool HasFriendPresence()const{return friendRoster_.KnownFor(social_);}
    std::size_t FriendsPlaying(std::uint32_t title)const{return HasFriendPresence()?InfinitySocial::FriendsPlayingTitle(friendRoster_.Rows(),title):0;}
    InfinitySocial::NativeAction TakeNativeSocialRequest(unsigned* title=nullptr,std::uint64_t* target=nullptr){
        const auto request=nativeSocialQueue_.Take(social_);
        if(title)*title=0;
        if(target)*target=0;
        if(InfinitySocial::TargetedSocialAction(request.action)){
            if(!target||!FriendGuideOpen()||!friendFreshness_.Fresh(TransferRate::Now())||
               !friendRoster_.ContainsTarget(social_,request.target,request.action))
                return InfinitySocial::NativeAction::None;
            *target=request.target;
        }
        if(title)*title=request.title;
        return request.action;
    }
    void NativeSocialResult(const std::string& message){socialPolled_=false;socialArtPolled_=false;privileges_.Clear();avatarPreview_.Clear();avatarRefresh_.Clear();avatarManifest_={};avatarBudget_={};notice_=message;}
    void NativeSocialSignal(const char* message){if(message)RecordEvent(message);}
    bool TakeSigninRequest(){const bool pending=signinPending_;signinPending_=false;return pending;}
    void SigninResult(bool accepted){nativeSocialQueue_.Clear();socialPolled_=false;socialArtPolled_=false;privileges_.Clear();avatarPreview_.Clear();avatarRefresh_.Clear();avatarManifest_={};avatarBudget_={};notice_=accepted?"Selecao de perfil solicitada ao Xbox.":"O Xbox nao abriu a selecao de perfil.";}
    const std::string& QuickGuideOrder()const{return quickGuideOrder_;}
    bool IdleEligible()const{return page_==Home&&approvedGroup_<0&&!weatherPanel_&&!quickGuideOpen_&&!homeEditing_&&!importPreview_&&!storeActive_&&!searchActive_&&!TransferRunning()&&!ftp_.Running()&&!peerProbe_.Running();}
    std::string Press(Key key);  // Only confirmation on an installed title supplies a launch path.
    void Draw(HomeCanvas& canvas, const std::string& clock, int64_t now=-1) const;
    std::string TakeMovie();
    bool Tick();
    void StopFtp() {ftp_.Stop();}
    bool AvatarLaunchBusy()const{return TransferRunning()||ftp_.Running()||peerProbe_.Running();}
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
    bool TakeRecoverRequest(ContentBackup::Record& selected,bool& verifyOnly);
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
    void RecordLaunchFailure(const std::string& path);
    LaunchStatistics::Entry LaunchStats(const std::string& path)const{return launchStats_.Get(path);}
private:
    enum Section { Library, Iptv, Apps, Emulators, Files, System, Recent, Favorites };
    enum Page { Home, Listing, Details };
    std::string dir_, notice_;
    LaunchStatistics launchStats_;
    bool quickGuideOpen_=false,quickGuideEditing_=false;
    int quickGuideRow_=0;
    bool socialGuideFocus_=false;
    int socialGuideRow_=0;
    bool friendGuideOpen_=false,friendRosterPending_=false;
    bool avatarGuideOpen_=false,avatarBodyPending_=false;
    InfinitySocial::ProfileArt profileArt_=InfinitySocial::ProfileArt::Avatar;
    bool communicationGuideOpen_=false;
    bool privilegeGuideOpen_=false,privilegePending_=false;
    InfinitySocial::ProfilePrivileges privileges_;
    void DrawPrivilegeGuide(HomeCanvas& c)const;
    int communicationRow_=0;
    void DrawCommunicationGuide(HomeCanvas& c)const;
    void PressCommunicationGuide(Key key);
    InfinitySocial::AvatarCapabilities avatarCapabilities_;
    InfinitySocial::AvatarPreview avatarPreview_;
    InfinitySocial::AvatarRefresh avatarRefresh_;
    InfinitySocial::AvatarManifestSummary avatarManifest_;
    InfinitySocial::AvatarAssetBudget avatarBudget_;
    void DrawAvatarGuide(HomeCanvas& c)const;
    std::size_t friendRow_=0;
    InfinitySocial::FriendFilter friendFilter_=InfinitySocial::FriendFilter::All;
    InfinitySocial::FriendRoster friendRoster_;
    InfinitySocial::FriendFreshness friendFreshness_;
    bool friendAutoEnabled_=false;
    void DrawFriendGuide(HomeCanvas& c)const;
    void PressFriendGuide(Key key);
    InfinitySocial::AvatarExitConfirmation avatarExitConfirm_;
    InfinitySocial::NativeRequestQueue nativeSocialQueue_;
    void OpenProfileGuide();
    int quickGuideEditStartRow_=0;
    std::string quickGuideOrder_=QuickGuide::Default(),quickGuideDraft_;
    void OpenQuickGuideAction(unsigned action);
    void DrawQuickGuide(HomeCanvas& c)const;
    void DrawSocialHeader(HomeCanvas& c,int x,int y,int w)const;
    void DrawSocialGuide(HomeCanvas& c)const;
    InfinitySocial::Header social_;
    InfinitySocial::SessionEvents socialEvents_;
    InfinitySocial::CounterEvents socialCounters_;
    uint64_t lastSocialPoll_=0;
    uint64_t lastSocialArtPoll_=0;
    bool socialArtPolled_=false;
    bool socialPolled_=false,signinPending_=false;
    std::vector<std::string> roots_, baseRoots_, history_, favorites_, pinned_, hidden_, launched_, folders_;
    std::vector<Game> games_, tools_;
    InfinityPs1::Index ps1Covers_;
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
    TransferRate copyRate_;
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
    NotificationCenter notifications_;
    NotificationCenter::Filter eventFilter_=NotificationCenter::All;
    int eventRow_=0;
    void MarkNotifications(bool all);
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
    LanPeerProbe peerProbe_;
    bool networkPeer_=false,peerEditing_=false,peerOptions_=false;
    unsigned peerMode_=0,peerDuration_=0;
    unsigned peerOctet_=0;
    LanPeerConfig::Octets peerDraft_{{192,168,1,2}};
    void RefreshNetwork();
    SmbProbe smbProbe_;
    bool smbPage_=false,smbEditing_=false;
    unsigned smbOctet_=0;
    LanPeerConfig::Octets smbDraft_{{192,168,1,2}};
    void DrawSmb(HomeCanvas& c)const;
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    enum SmbUi { SmbClosed,SmbCredentials,SmbConnecting,SmbBrowsing,SmbDestination,SmbCopying,SmbError };
    SmbUi smbUi_=SmbClosed;
    SmbConsoleStage smbStage_;
    SmbReadSession smbSession_;
    SmbBrowser smbBrowser_;
    SmbBrowser::CopyPlan smbPlan_;
    std::string smbCredentials_[4];
    unsigned smbField_=0,smbChar_=0,smbDest_=0;
    bool smbConfirm_=false;
    std::vector<std::string> smbDestinations_;
    std::string smbMessage_;
    void CloseSmbSession();
    void PressSmbSession(Key key);
    bool TickSmbSession();
    void DrawSmbSession(HomeCanvas& c)const;
    bool SmbEnabled(bool writes=false)const;
    void WipeSmbCredentials();
#endif
    SystemTelemetry telemetry_;
    VideoTelemetry videoTelemetry_;
    bool telemetryVideo_=false,thermalPage_=false;
    ThermalTelemetry thermal_,headerThermal_;
    uint64_t headerThermalAt_=0;
    bool headerThermalSampled_=false;
    ThermalHistory thermalHistory_;
    bool coolingPage_=false;
    unsigned thermalLimit_=0;
    void DrawCooling(HomeCanvas& c)const;
    void DrawThermal(HomeCanvas& c)const;
    void DrawTelemetry(HomeCanvas& c) const;
    void RefreshResidentPlugins();
    void DrawResidentPlugins(HomeCanvas& c) const;
    std::vector<ResidentConfig> residentConfigs_;
    size_t residentSource_=0;
    bool localPlugins_=false;
    int localPluginRow_=0;
    std::vector<PluginDescriptor> localPluginItems_;
    std::set<std::string> disabledPluginIds_;
    void RefreshLocalPlugins();
    void DrawLocalPlugins(HomeCanvas& c)const;
    bool PluginAllowed(const PluginDescriptor& item)const;
    void ToggleLocalPlugin();
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
    std::vector<std::string> configuredRoots_,disabledRoots_,luaScripts_;
    bool libraryPathsPage_=false,luaPage_=false;
    int libraryPathRow_=0,luaRow_=0;
    void DrawLibraryPaths(HomeCanvas& c)const;
    void DrawLuaScripts(HomeCanvas& c)const;
    void RefreshLuaScripts();
    void ToggleLibraryRoot();
    std::vector<GameScanner::Depth> scanDepths_;
    void ChangeLibraryDepth(int delta);
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
    std::string storeSource_="*";
    std::string StoreSourceLabel()const;
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
    bool verifyOnly_=false;
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
    ClockWeather::Settings clockSettings_,clockDraft_;
    ClockWeather::Reading weather_;
    bool weatherPending_=false,weatherPanel_=false;
    int64_t lastWeatherAttempt_=0;
    void DrawWeatherPanel(HomeCanvas& c)const;
    unsigned theme_=0,safeArea_=0,idleMinutes_=0,language_=0;
    AppearanceSettings appearance_,appearanceDraft_;
    unsigned initialLayout_=1,layout_=1,layoutDraft_=1;
    unsigned gridPreference_=0,gridDraft_=0;
    bool gridFilter_=false;
    bool GridListing()const;
    void DrawGameGrid(HomeCanvas& c)const;
    void DrawModernGameDetails(HomeCanvas& c)const;
    void DrawModernStore(HomeCanvas& c)const;
    bool layoutPreview_=false;
    void DrawModernHome(HomeCanvas& c,unsigned style,bool preview=false)const;
    void DrawApprovedHome(HomeCanvas& c,bool preview=false)const;
    void DrawApprovedHeader(HomeCanvas& c,bool weatherFocused)const;
    std::vector<const Game*> ApprovedShelf()const;
    size_t approvedPage_=0,approvedHero_=0;
    std::string approvedReversePath_;
    std::string ReverseArtPath(const Game& game)const;
    uint64_t approvedHeroAt_=0;
    std::string approvedShortcuts_="012345",approvedDraft_;
    bool ApprovedCarouselTick(uint64_t now);
    bool PressApprovedHome(Key key);
    void OpenApprovedSlot();
    void OpenApprovedAction(int action);
    void DrawApprovedGroup(HomeCanvas& c)const;
    int approvedGroup_=-1,approvedGroupRow_=0;
    unsigned themeDraft_=0,appearanceRow_=0;
    bool appearanceEditor_=false;
    mutable bool wallpaperDirty_=true;
    void PressAppearance(Key key);
    void DrawAppearance(HomeCanvas& c)const;
    bool sessionExitConfirm_=false;
    void DrawConsoleStatus(HomeCanvas& c) const;
    void DrawLocalMilestones(HomeCanvas& c)const;
    bool localMilestones_=false;
    bool achievementsPage_=false,achievementDetails_=false,achievementLoaded_=false;
    bool nativeAchievements_=false,nativeTitlesLimited_=false;
    size_t nativeAchievementRow_=0;
    std::vector<std::pair<unsigned,std::string>> nativeAchievementTitles_;
    void RefreshNativeAchievementTitles();
    size_t achievementRow_=0,achievementDetailRow_=0;
    GpdAchievements::Summary achievementSummary_;
    std::vector<GpdAchievements::Achievement> achievementItems_;
    void RefreshAchievements();
    void PressAchievements(Key key);
    void DrawAchievements(HomeCanvas& c)const;
    bool saveCenter_=false,saveList_=false,saveBackups_=false,saveConfirm_=false,saveLimited_=false;
    size_t saveProfileRow_=0,saveRow_=0,saveBackupRow_=0;
    unsigned saveTitle_=0;
    std::vector<XboxSaveProfile> saveProfiles_;
    std::vector<XboxContentPackage> saveItems_;
    std::vector<ContentBackup::Record> saveRecords_;
    void RefreshSaveProfiles();
    void RefreshSaveItems();
    void RefreshSaveRecords();
    void PressSaveCenter(Key key);
    void DrawSaveCenter(HomeCanvas& c)const;
    bool powerPage_=false;
    ConsolePower::Confirmation powerConfirm_;
    bool PowerBusy()const;
    void DrawPower(HomeCanvas& c)const;
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
    bool artArchiveConfirm_=false,artArchiveRestore_=false;
    int artEditRow_;
    std::string artEditFolder_, artEditId_, artDraft_;
    std::string ArtPath(const Game& game,bool cover) const;
    void DrawArtEditor(HomeCanvas& c) const;
    std::vector<XboxContentPackage> contentItems_;
    std::vector<TrainerEntry> trainers_;
    size_t fpsReferenceCount_=0;
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
