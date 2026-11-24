#include "FileWorkspace.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs=std::filesystem;
int main() {
    const auto root=fs::temp_directory_path()/("aurorax-files-"+std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    const std::string app=root.string()+"/";
    FileWorkspace files(app);
    assert(files.CanBrowse(app));
    assert(!files.CanWrite(app));
    assert(!files.Create(app,"Games"));
    assert(!files.Create(app,"../escape"));
    assert(files.Create(app,"UserFiles"));
    assert(!files.Create(app,"UserFiles"));
    const std::string user=(root/"UserFiles").string();
    assert(files.CanWrite(user));
    assert(files.Create(user,"Documents"));
    assert(!files.Create(user,".."));
    const std::string docs=(root/"UserFiles"/"Documents").string();
    std::ofstream(root/"test.txt")<<"preserve original";
    assert(files.Copy(app,"test.txt",docs));
    assert(!files.Copy(app,"test.txt",docs));
    assert(!files.Rename(app,"test.txt","other.txt"));
    assert(files.Rename(docs,"test.txt","renamed.txt"));
    assert(fs::exists(root/"test.txt"));
    assert(fs::exists(fs::path(docs)/"renamed.txt"));
    assert(!fs::exists(fs::path(docs)/"test.txt"));
    assert(!files.Rename(docs,"renamed.txt","../test.txt"));
    assert(!files.Copy(app,"test.txt",app));
    assert(!files.Copy(app,"UserFiles",docs));
    {std::ofstream big(root/"big.bin",std::ios::binary);
     big.seekp(8*1024*1024);big.put('x');}
    assert(!files.Copy(app,"big.bin",docs));
#ifndef _WIN32
    std::error_code ec;
    fs::create_directory_symlink(root,fs::path(docs)/"link",ec);
    if(!ec) {
        assert(!files.CanBrowse((fs::path(docs)/"link").string()));
        assert(!files.Create((fs::path(docs)/"link").string(),"unexpected"));
    }
#endif
    // Incremental transfer: larger than the legacy 8 MiB limit.
    FileTransfer job;
    assert(job.Start(files,app,"big.bin",docs));
    assert(job.Status()==FileTransfer::Running && job.Total()==8*1024*1024+1);
    assert(!fs::exists(fs::path(docs)/"big.bin"));
    job.Tick();
    assert(job.Done()==64*1024 && job.Status()==FileTransfer::Running);
    job.Cancel();
    assert(job.Status()==FileTransfer::Cancelled);
    assert(!fs::exists(fs::path(docs)/"big.bin"));
    assert(fs::exists(root/"big.bin"));
    for(const auto& entry:fs::directory_iterator(docs))
        assert(entry.path().filename().string().find("AXCOPY-")!=0);

    // A stale part from another session must never be overwritten or removed.
    const auto stale=fs::path(docs)/"AXCOPY-0000.part";
    std::ofstream(stale)<<"preserve";
    assert(job.Start(files,app,"big.bin",docs));
    unsigned last=0;
    int steps=0;
    while(job.Status()==FileTransfer::Running && steps++<1000) {
        job.Tick();
        assert(job.Percent()>=last && job.Percent()<=100);
        last=job.Percent();
    }
    assert(job.Status()==FileTransfer::Completed && job.Percent()==100);
    assert(fs::file_size(fs::path(docs)/"big.bin")==fs::file_size(root/"big.bin"));
    assert(fs::file_size(stale)==8);
    assert(!job.Start(files,app,"big.bin",docs));

    // Destination appears during the copy: never replace it.
    std::ofstream(root/"collision.txt")<<"source";
    assert(job.Start(files,app,"collision.txt",docs));
    std::ofstream(fs::path(docs)/"collision.txt")<<"original destination";
    while(job.Status()==FileTransfer::Running)job.Tick();
    assert(job.Status()==FileTransfer::Failed);
    assert(fs::file_size(fs::path(docs)/"collision.txt")==20);
    assert(fs::file_size(root/"collision.txt")==6);

    // Empty file and destructor cancellation.
    std::ofstream(root/"empty.txt").close();
    assert(job.Start(files,app,"empty.txt",docs));
    job.Tick();
    assert(job.Status()==FileTransfer::Completed);
    assert(fs::file_size(fs::path(docs)/"empty.txt")==0);
    std::ofstream(root/"abandon.txt")<<"keep";
    {FileTransfer abandoned;assert(abandoned.Start(files,app,"abandon.txt",docs));}
    assert(!fs::exists(fs::path(docs)/"abandon.txt"));
    assert(fs::exists(root/"abandon.txt"));
    assert(!files.ValidName("AXCOPY-0000.part"));
    for(const auto& entry:fs::directory_iterator(docs))
        if(entry.path().filename().string().find("AXCOPY-")==0)
            assert(entry.path()==stale);

    assert(files.Create(user,"MoveTo"));
    const std::string moveTo=(fs::path(user)/"MoveTo").string();
    assert(files.Move(docs,"renamed.txt",moveTo));
    assert(!fs::exists(fs::path(docs)/"renamed.txt"));
    assert(fs::exists(fs::path(moveTo)/"renamed.txt"));
    std::ofstream(fs::path(docs)/"renamed.txt")<<"collision";
    assert(!files.Move(moveTo,"renamed.txt",docs));
    assert(fs::exists(fs::path(moveTo)/"renamed.txt"));
    assert(!files.Move(app,"test.txt",moveTo));
    assert(!files.Remove(app,"test.txt"));
    assert(!files.Remove(user,"MoveTo")); // nonempty directory
    assert(!files.Remove(docs,"AXCOPY-0000.part"));
    assert(files.Remove(moveTo,"renamed.txt"));
    assert(files.Remove(user,"MoveTo"));
    assert(!fs::exists(fs::path(user)/"MoveTo"));
    // Bounded, read-only plan for nested directories, empty folders and files.
    fs::create_directories(root/"Tree"/"Nested"/"Empty");
    std::ofstream(root/"Tree"/"Nested"/"file.txt")<<"content";
    std::vector<WorkspaceCopyStep> plan;
    assert(files.PlanCopies(app,std::vector<std::string>(1,"Tree"),docs,plan));
    assert(plan.size()==4 && plan[0].directory && plan[0].name=="Tree");
    assert(!fs::exists(fs::path(docs)/"Tree"));
    assert(files.Create(docs,"Tree"));
    assert(!files.PlanCopies(app,std::vector<std::string>(1,"Tree"),docs,plan));
    assert(plan.empty());
    assert(files.Create(user,"Outer"));
    const std::string outer=(fs::path(user)/"Outer").string();
    assert(files.Create(outer,"Inner"));
    assert(!files.PlanCopies(user,std::vector<std::string>(1,"Outer"),
                             (fs::path(outer)/"Inner").string(),plan));
#ifndef _WIN32
    std::error_code copyLinkError;
    fs::create_directory_symlink(root,root/"Tree"/"Link",copyLinkError);
    if(!copyLinkError)assert(!files.PlanCopies(app,std::vector<std::string>(1,"Tree"),user,plan));
#endif
    fs::remove_all(root);
}
