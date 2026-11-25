#include "StorePackage.h"
#include "TransferRate.h"
#include "DiscInstaller.h"
#include <cassert>
#include <filesystem>
#include <fstream>
namespace fs=std::filesystem;
int main(){
    TransferRate rate;
    assert(rate.Sample(500,4500,100)); // resume at 500 bytes
    assert(rate.BytesPerSecond()==0);
    assert(!rate.Sample(1000,4500,500));
    assert(rate.Sample(1500,4500,1100));
    assert(rate.BytesPerSecond()==1000&&rate.RemainingSeconds(1500,4500)==3);
    assert(rate.Sample(1500,4500,2100)&&rate.BytesPerSecond()==0);
    assert(rate.Sample(0,4500,3100)&&rate.BytesPerSecond()==0);
    assert(rate.Sample(1000,4500,4100)&&rate.BytesPerSecond()==1000);
    assert(rate.Sample(1000,9000,4200)&&rate.BytesPerSecond()==0);
    rate.Reset();assert(rate.BytesPerSecond()==0);

    StoreSha256 empty;assert(empty.Finish()=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    StoreSha256 abc;abc.Update((const unsigned char*)"a",1);abc.Update((const unsigned char*)"bc",2);
    const std::string digest="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    assert(abc.Finish()==digest);
    StoreSha256 million;std::string block(1000,'a');
    for(int i=0;i<1000;++i)million.Update((const unsigned char*)block.data(),block.size());
    assert(million.Finish()=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    fs::path root=fs::temp_directory_path()/"infinity-store-package-test";
    fs::remove_all(root);fs::create_directories(root);
    std::string base=root.string()+"/";
    FileWorkspace workspace(base);StorePackage out(workspace);
    StoreItem item;item.id="demo";item.bytes=3;item.sha256=digest;item.url="https://example.org/demo";
    assert(!StoreNativeSource(item,base));
    {std::ofstream f(root/"infinity360-store-hosts.txt");f<<"example.org\n";}
    assert(StoreNativeSource(item,base));
    item.url="https://other.example/demo";assert(!StoreNativeSource(item,base));
    assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"bad",3));assert(!out.Finish());
    assert(!fs::exists(root/"UserFiles/Store/demo.pkg"));
    assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"abc",3));assert(out.Finish());
    assert(fs::file_size(root/"UserFiles/Store/demo.pkg")==3);
    StoreCatalog cat(base);assert(cat.Progress(item).state==StoreProgress::NativeVerified);
    assert(!cat.Requested(item));assert(cat.ListedDownload(item));
    StoreItem unknown=item;unknown.id="unknown";assert(!cat.ListedDownload(unknown));
    assert(cat.Load()==false); // missing catalog, but requests storage is initialized
    assert(cat.Request(unknown));assert(cat.ListedDownload(unknown));
    assert(cat.Cancel(unknown));assert(!cat.ListedDownload(unknown));
    assert(!out.Begin(item,base));
    item.id="cancel";assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"a",1));out.Cancel();
    assert(!fs::exists(root/"UserFiles/Store/cancel.pkg"));
    item.id="race";assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"abc",3));
    {std::ofstream f(root/"UserFiles/Store/race.pkg");f<<"original";}
    assert(!out.Finish());assert(fs::file_size(root/"UserFiles/Store/race.pkg")==8);
    for(auto& entry:fs::directory_iterator(root/"UserFiles/Store"))assert(entry.path().extension()!=".part");
#ifndef _WIN32
    item.id="resume";
    {StorePackage first(workspace);assert(first.Begin(item,base,true));assert(first.Write((const unsigned char*)"a",1));assert(first.Pause());}
    assert(cat.Progress(item).state==StoreProgress::NativePaused);
    assert(cat.Progress(item).received==1);assert(cat.ListedDownload(item));
    {StorePackage resumed(workspace);assert(resumed.Begin(item,base,true));assert(resumed.Done()==1);assert(resumed.Write((const unsigned char*)"bc",2));assert(resumed.Finish());}
    assert(cat.Progress(item).state==StoreProgress::NativeVerified);
    item.id="resume-bad";
    {StorePackage first(workspace);assert(first.Begin(item,base,true));assert(first.Write((const unsigned char*)"x",1));assert(first.Pause());}
    {StorePackage resumed(workspace);assert(resumed.Begin(item,base,true));assert(resumed.Write((const unsigned char*)"bc",2));assert(!resumed.Finish());}
    assert(!cat.Staged(item));assert(!cat.ListedDownload(item));
    item.id="discard";
    {StorePackage first(workspace);assert(first.Begin(item,base,true));assert(first.Write((const unsigned char*)"a",1));assert(first.Pause());}
    assert(cat.DiscardPartial(item));assert(!cat.ListedDownload(item));
#endif
    // The native queue is independent of the PC request queue, ordered and
    // keyed by exact catalog identity. Reload does not auto-start downloads.
    StoreItem one=item;one.id="first";one.url="https://example.org/first";
    StoreItem two=one;two.id="second";
    {std::ofstream catalog(root/"aurorax-store.tsv");
     for(const auto& q:{one,two})catalog<<q.id<<"\thomebrew\tTest\t1\t3\t"<<q.sha256<<"\t"<<q.url<<"\tfixture\n";}
    assert(cat.Load());assert(cat.QueueNative(two));assert(cat.QueueNative(one));assert(cat.QueueNative(two));
    assert(!cat.Requested(two));assert(cat.ListedDownload(two));
    StoreCatalog reloaded(base);assert(reloaded.Load());
    auto ordered=reloaded.NativeQueue();assert(ordered.size()==2);
    assert(reloaded.Items()[ordered[0]].id=="second");
    assert(reloaded.Items()[ordered[1]].id=="first");
    auto summary=reloaded.QueueSummary();assert(summary.queued==2&&summary.remaining==6&&summary.unavailable==0);
    assert(reloaded.PrioritizeNative(one));assert(reloaded.PrioritizeNative(one));
    assert(!reloaded.PrioritizeNative(item)); // unrelated identity
    StoreCatalog prioritized(base);assert(prioritized.Load());
    ordered=prioritized.NativeQueue();assert(ordered.size()==2&&prioritized.Items()[ordered[0]].id=="first");
    assert(reloaded.Load());
    assert(reloaded.RemoveNative(two));assert(!reloaded.NativeQueued(two));assert(reloaded.NativeQueued(one));
    StoreItem changed=one;changed.sha256=std::string(64,'f');assert(!reloaded.NativeQueued(changed));
    {std::ofstream broken(root/"infinity360-native-queue.tsv");broken<<"malformed\n";}
    assert(reloaded.Load());assert(!reloaded.NativeQueueValid());assert(!reloaded.QueueNative(two));assert(!reloaded.PrioritizeNative(two));
#ifndef _WIN32
    // Synthetic structural XEX fixture: no executable payload, never launched.
    std::string fixture(8192,'\0');
    fixture.replace(0,4,"XEX2");fixture[10]=16;fixture[19]=32;
    StoreItem app=one;app.id="installer";app.category="homebrew";app.bytes=fixture.size();app.rights="test fixture";
    StoreSha256 appHash;appHash.Update(reinterpret_cast<const unsigned char*>(fixture.data()),fixture.size());app.sha256=appHash.Finish();
    {std::ofstream file(root/"UserFiles/Store/installer.pkg",std::ios::binary);file.write(fixture.data(),fixture.size());}
    const auto go=[](unsigned long long,unsigned long long){return true;};
    std::string error;
    assert(!StoreInstallXex(workspace,app,base,go,error));
    {std::ofstream flag(root/"enable-store-install.txt");flag<<"ENABLE-STORE-INSTALL\n";}
    auto denied=app;denied.category="game";assert(!StoreInstallXex(workspace,denied,base,go,error));
    auto wrong=app;wrong.sha256=std::string(64,'0');assert(!StoreInstallXex(workspace,wrong,base,go,error));
    assert(!cat.Installed(app));
    assert(!StoreInstallXex(workspace,app,base,[](unsigned long long,unsigned long long){return false;},error));
    assert(!cat.Installed(app));
    assert(StoreInstallXex(workspace,app,base,go,error));assert(cat.Installed(app));
    assert(!StoreInstallXex(workspace,app,base,go,error));
    assert(fs::file_size(root/"UserFiles/Homebrew/installer/default.xex")==app.bytes);
    assert(fs::exists(root/"UserFiles/Store/installer.pkg"));
#endif
#if defined(__linux__) && !defined(__XSDK__)
    // Host fixture for the exposed DVD filesystem. Never uses a real game.
    fs::create_directories(root/"disc"/"data");
    auto put=[&](size_t at,unsigned value){for(int i=3;i>=0;--i){fixture[at+i]=static_cast<char>(value&255);value>>=8;}};
    put(20,1);put(24,0x00040006);put(28,64);put(64,0x11223344);put(76,0x494E4635);
    fixture[82]=1;fixture[83]=1;
    {std::ofstream file(root/"disc/default.xex",std::ios::binary);file.write(fixture.data(),fixture.size());}
    std::ofstream(root/"disc/data/resource.bin",std::ios::binary)<<"disc fixture";
    std::string installed;
    assert(!InstallDiscFiles(workspace,base,(root/"disc").string(),base,go,installed,error));
    {std::ofstream marker(root/"enable-dvd-install.txt");marker<<"ENABLE-DVD-INSTALL\n";}
    assert(!InstallDiscFiles(workspace,base,(root/"disc").string(),base,[](unsigned long long,unsigned long long){return false;},installed,error));
    unsigned scans=0;
    assert(!InstallDiscFiles(workspace,base,(root/"disc").string(),base,[&](unsigned long long,unsigned long long){return ++scans<3;},installed,error));
    XboxExecutionInfo expected{};
    assert(ContentCatalog::ReadExecutionInfo((root/"disc/default.xex").string(),expected));
    XboxExecutionInfo wrongDisc=expected;wrongDisc.discNumber=2;
    assert(!DiscIdentity::Same(expected,wrongDisc));
    assert(!InstallDiscFiles(workspace,base,(root/"disc").string(),base,go,installed,error,&wrongDisc));
    assert(InstallDiscFiles(workspace,base,(root/"disc").string(),base,go,installed,error,&expected));
    assert(fs::file_size(fs::path(installed)/"default.xex")==fixture.size());
    assert(fs::file_size(fs::path(installed)/"data/resource.bin")==12);
    assert(!InstallDiscFiles(workspace,base,(root/"disc").string(),base,go,installed,error));
#endif
    // Isolate the new provider scenario from the deliberately malformed queue fixture.
    fs::remove(root/"infinity360-native-queue.tsv");
    fs::remove(root/"infinity360-native-queue.tsv.bak");
    // Pinned remote snapshots: exact digest, namespace, offline reload, tamper detection.
    const std::string catalogRow="demo\thomebrew\tRemote Demo\t1\t3\t"+digest+"\thttps://example.org/demo\tAuthorized fixture\n";
    StoreSha256 pin;pin.Update(reinterpret_cast<const unsigned char*>(catalogRow.data()),catalogRow.size());
    const std::string pinned=pin.Finish();
    {std::ofstream config(root/"infinity360-providers.tsv");
     config<<"test\tTest provider\thttps://example.org/catalog.tsv\t"<<catalogRow.size()<<"\t"<<pinned<<"\n";}
    StoreCatalog remote(base);assert(remote.Load());assert(remote.Providers().size()==1);
    StoreProvider provider=remote.Providers()[0];assert(!provider.cached);
    std::string providerError;
    assert(!remote.ImportProvider(provider,catalogRow+"x",providerError));
    assert(remote.ImportProvider(provider,catalogRow,providerError));
    assert(remote.Providers()[0].cached&&remote.Providers()[0].items==1);
    StoreItem entry=remote.Items().back();assert(entry.id=="test-demo"&&entry.provider=="test");
    assert(!remote.Request(entry));assert(remote.QueueNative(entry));
    StoreCatalog offline(base);assert(offline.Load());assert(offline.Providers()[0].cached);
    {std::ofstream corrupt(root/"UserFiles/StoreCatalogs/test.tsv");corrupt<<catalogRow<<"x";}
    assert(offline.Load());assert(!offline.Providers()[0].cached);
    assert(offline.ImportProvider(provider,catalogRow,providerError));
    {std::ofstream revoke(root/"infinity360-providers.tsv");revoke<<"# removed\n";}
    assert(!offline.ImportProvider(provider,catalogRow,providerError));
    assert(offline.Load());assert(offline.Providers().empty());
    assert(offline.OrphanNativeCount()==1);assert(offline.PruneOrphanNative());assert(offline.OrphanNativeCount()==0);
    assert(offline.Load());assert(offline.OrphanNativeCount()==0);
    fs::remove_all(root);
}
